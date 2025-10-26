#include "ga.hpp"

#include "genetic_algorithm.hpp"

#include "object_pools.hpp"
#include "eax_tabu.hpp"
#include "greedy_evaluator.hpp"
#include "entropy_evaluator.hpp"
#include "distance_preserving_evaluator.hpp"

#include "generational_model.hpp"

namespace eax {
std::pair<mpi::genetic_algorithm::TerminationReason, std::vector<Individual>> execute_ga(std::vector<Individual>& population, Context& context, std::chrono::system_clock::time_point timeout_time) {
    using namespace std;
    using Context = eax::Context;
    // オブジェクトプール
    eax::ObjectPools object_pools(context.env.tsp.city_count);
    
    // 交叉関数
    eax::EAX_tabu_Rand eax_tabu_rand(object_pools);
    eax::EAX_tabu_N_AB eax_tabu_n_ab(object_pools);
    auto crossover_func = [&eax_tabu_rand, &eax_tabu_n_ab](const Individual& parent1, const Individual& parent2,
                                Context& context) {
        auto& env = context.env;
        struct {
            eax::EAX_tabu_Rand& eax_tabu_rand;
            eax::EAX_tabu_N_AB& eax_tabu_n_ab;
            const Individual& parent1;
            const Individual& parent2;
            Context& context;
            auto operator()(const eax::EAXType& type) {
                switch (type) {
                    case eax::EAXType::EAX_Rand:
                        return eax_tabu_rand(parent1, parent2, context.env.num_children, parent1.get_tabu_edges(), context.env.tsp, context.random_gen);
                    default:
                        throw std::runtime_error("Unknown EAX type.");
                }
            }
            
            auto operator()(const EAX_n_AB& n_ab) {
                return eax_tabu_n_ab(parent1, parent2, context.env.num_children, parent1.get_tabu_edges(), context.env.tsp, context.random_gen, n_ab.n);
            }
        } visitor {eax_tabu_rand, eax_tabu_n_ab, parent1, parent2, context};
        
        return std::visit(visitor, env.eax_type);
    };

    // 適応度関数
    auto calc_fitness_lambda = [](const eax::CrossoverDelta& child, Context& context) {
        auto& env = context.env;
        switch (env.selection_type) {
            case eax::SelectionType::Greedy:
                return eax::eval::delta::Greedy()(child, env.tsp.adjacency_matrix);
            case eax::SelectionType::Ent:
                return eax::eval::delta::Entropy()(child, env.tsp.adjacency_matrix, context.pop_edge_counts, env.population_size);
            case eax::SelectionType::DistancePreserving:
                return eax::eval::delta::DistancePreserving()(child, env.tsp.adjacency_matrix, context.pop_edge_counts);
            default:
                throw std::runtime_error("Unknown selection type");
        }
    };
    
    // 更新処理関数
    struct {
        std::chrono::system_clock::time_point timeout_time;
        mpi::genetic_algorithm::TerminationReason operator()(vector<Individual>& population, Context& context, size_t generation) {
            context.current_generation = generation;

            // Greedy Selection以外はエッジカウントを個体の評価に使用するため、個体更新時にエッジカウントも更新する
            if (context.env.selection_type != eax::SelectionType::Greedy) {
                update_individual_and_edge_counts(population, context);
            } else {
                update(population, context);
            }
            
            if (std::chrono::system_clock::now() >= timeout_time) {
                return mpi::genetic_algorithm::TerminationReason::TimeLimit;
            }

            return continue_condition(population, context, generation);
        }
        
        void update(vector<Individual>& population, Context& context) {
            for (auto& individual : population) {
                individual.update(context);
            }
        }
        
        void update_individual_and_edge_counts(vector<Individual>& population, Context& context) {
            for (auto& individual : population) {
                auto delta = individual.update(context);
                for (const auto& mod : delta.get_modifications()) {
                    size_t v1 = mod.edge1.first;
                    size_t v2 = mod.edge1.second;
                    size_t new_v2 = mod.new_v2;
                    context.pop_edge_counts[v1][v2] -= 1;
                    context.pop_edge_counts[v1][new_v2] += 1;
                }
            }
        }

        mpi::genetic_algorithm::TerminationReason continue_condition(const vector<Individual>& population, Context& context, size_t generation) {
            double best_length = std::numeric_limits<double>::max();
            double average_length = 0.0;
            for (size_t i = 0; i < population.size(); ++i) {
                double length = population[i].get_distance();
                best_length = std::min(best_length, length);
                average_length += length;
            }
            average_length /= population.size();
            
            if (best_length < context.best_length) {
                context.best_length = best_length;
                context.generation_of_reached_best = generation;
                context.stagnation_generations = 0;
            }else {
                context.stagnation_generations += 1;
            }
            
            if (average_length - best_length < 0.001)
                return mpi::genetic_algorithm::TerminationReason::Converged; // 収束条件
            
            const size_t N_child = context.env.num_children;
            
            if (context.stagnation_generations >= (3000 / N_child)) {
                return mpi::genetic_algorithm::TerminationReason::Stagnation; // 停滞条件
            }
            
            return mpi::genetic_algorithm::TerminationReason::NotTerminated;
        }
    } update_func {timeout_time};
    
    // ロガー
    struct {
        void operator()([[maybe_unused]]const vector<Individual>& population, Context& context, size_t generation) {
            std::vector<double> lengths(population.size());
            for (size_t i = 0; i < population.size(); ++i) {
                lengths[i] = population[i].get_distance();
            }
            double best_length = *std::min_element(lengths.begin(), lengths.end());
            double average_length = std::accumulate(lengths.begin(), lengths.end(), 0.0) / lengths.size();
            double worst_length = *std::max_element(lengths.begin(), lengths.end());
            cout << "Generation " << generation << ": Best Length = " << best_length 
                    << ", Average Length = " << average_length << ", Worst Length = " << worst_length << endl;
            
            if (context.start_time.time_since_epoch().count() == 0) {
                // 計測開始時刻が未設定なら、現在時刻を設定
                const_cast<Context&>(context).start_time = std::chrono::system_clock::now();
            } else {
                auto now = std::chrono::system_clock::now();
                context.elapsed_time += std::chrono::duration<double>(now - context.start_time).count();
                const_cast<Context&>(context).start_time = now;
            }
        }
    } logging;
    
    struct {
        void operator()([[maybe_unused]]const vector<Individual>& population, Context& context, size_t generation, [[maybe_unused]]mpi::genetic_algorithm::TerminationReason reason) {
            context.final_generation = generation;
        }
    } post_process;

    // 世代交代処理
    eax::GenerationalStep generational_step(calc_fitness_lambda, crossover_func);
    
    // GA実行オブジェクト
    eax::GenerationalModel genetic_algorithm(generational_step, update_func, logging, post_process);

    return genetic_algorithm.execute(population, context, context.current_generation);
}

Context create_context(const std::vector<Individual>& initial_population, Environment const& env) {
    Context context;
    context.env = env;

    context.set_initial_edge_counts(initial_population);
    context.random_gen = std::mt19937(env.random_seed);
    return context;
}

void serialize_population(const std::vector<Individual>& population, std::ostream& os) {
    os << "# Population" << std::endl;
    for (const auto& individual : population) {
        individual.serialize(os);
        os << std::endl;
    }
}

std::vector<Individual> deserialize_population(std::istream& is) {
    std::vector<Individual> population;
    std::string line;
    // # Population
    std::getline(is, line);
    if (line != "# Population") throw std::runtime_error("Expected '# Population'");
    while (std::getline(is, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        Individual individual = Individual::deserialize(iss);
        population.push_back(individual);
    }
    return population;
}
    
}