#include "ga.hpp"

#include <fstream>

#include "genetic_algorithm.hpp"

#include "object_pools.hpp"
#include "eax_n_ab.hpp"
#include "eax_block2.hpp"
#include "eax_rand.hpp"
#include "eax_uniform.hpp"
#include "greedy_evaluator.hpp"
#include "entropy_evaluator.hpp"
#include "distance_preserving_evaluator.hpp"
#include "generational_change_model.hpp"
#include "nagata_generation_change_model.hpp"


namespace eax {
std::pair<mpi::genetic_algorithm::TerminationReason, std::vector<Individual>> execute_ga(
    std::vector<Individual>& population,
    Context& context,
    const std::string& log_file_name) {

    using namespace std;
    using Context = eax::Context;
    // オブジェクトプール
    eax::ObjectPools object_pools(context.env.tsp.city_count);
    
    // 交叉関数
    eax::EAX_N_AB eax_n_ab(object_pools);
    eax::EAX_Block2 eax_block2(object_pools);
    eax::EAX_Rand eax_rand(object_pools);
    eax::EAX_UNIFORM eax_uniform(object_pools);
    auto crossover_func = [&eax_n_ab, &eax_block2, &eax_rand, &eax_uniform](const Individual& parent1, const Individual& parent2,
                                Context& context) {
        auto& env = context.env;
        
        struct {
            eax::EAX_N_AB& eax_n_ab;
            eax::EAX_Block2& eax_block2;
            eax::EAX_Rand& eax_rand;
            eax::EAX_UNIFORM& eax_uniform;
            const Individual& parent1;
            const Individual& parent2;
            Context& context;
            auto operator()(const eax::EAX_Rand_tag&) {
                return eax_rand(parent1, parent2, context.env.num_children, context.env.tsp, context.random_gen);
            }

            auto operator()(const eax::EAX_n_AB_tag& n_ab) {
                return eax_n_ab(parent1, parent2, context.env.num_children, context.env.tsp, context.random_gen, n_ab.get_n());
            }

            auto operator()(const eax::EAX_Block2_tag&) {
                return eax_block2(parent1, parent2, context.env.num_children, context.env.tsp, context.random_gen);
            }

            auto operator()(const eax::EAX_UNIFORM_tag& uniform) {
                return eax_uniform(parent1, parent2, context.env.num_children, context.env.tsp, context.random_gen, uniform.get_ratio());
            }
        } visitor {eax_n_ab, eax_block2, eax_rand, eax_uniform, parent1, parent2, context};
        
        return std::visit(visitor, env.eax_type);
    };

    // 適応度関数
    auto calc_fitness_lambda = [](const eax::CrossoverDelta& child, Context& context) {
        auto& env = context.env;
        switch (env.selection_type) {
            case eax::SelectionType::Greedy:
                return eax::eval::delta::Greedy(child);
            case eax::SelectionType::Ent:
                return eax::eval::delta::Entropy(child, context.pop_edge_counts, env.population_size);
            case eax::SelectionType::DistancePreserving:
                return eax::eval::delta::DistancePreserving(child, context.pop_edge_counts);
            default:
                throw std::runtime_error("Unknown selection type");
        }
    };
    
    // 更新処理関数
    struct {
        mpi::genetic_algorithm::TerminationReason operator()(vector<Individual>& population, Context& context, size_t generation) {
            context.current_generation = generation;

            update_individual_and_edge_counts(population, context);

            return continue_condition(population, context, generation);
        }
        
        void update_individual_and_edge_counts(vector<Individual>& population, Context& context) {
            for (auto& individual : population) {
                auto delta = individual.apply_pending_delta();
                auto delta_H = eax::calc_delta_entropy(delta, context.pop_edge_counts, context.env.population_size);
                context.entropy += delta_H;

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
            
            if (generation >= 10000)
                return mpi::genetic_algorithm::TerminationReason::MaxGenerations; // 最大世代数条件
            
            return mpi::genetic_algorithm::TerminationReason::NotTerminated;
        }
    } update_func;
    
    // ロガー
    std::ofstream log_file_stream;
    if (!log_file_name.empty()) {
        log_file_stream.open(log_file_name);
        log_file_stream << "Generation,BestLength,AverageLength,WorstLength,Entropy" << std::endl;
    }

    struct {
        std::ofstream& log_file_stream;

        void operator()([[maybe_unused]]const vector<Individual>& population, Context& context, size_t generation) {
            double time_per_generation = 0.0;

            if (context.start_time.time_since_epoch().count() == 0) {
                // 計測開始時刻が未設定なら、現在時刻を設定
                context.start_time = std::chrono::system_clock::now();
            } else {
                auto now = std::chrono::system_clock::now();
                time_per_generation = std::chrono::duration<double>(now - context.start_time).count();
                context.elapsed_time += time_per_generation;
                context.start_time = now;
            }


            if (!log_file_stream.is_open()) {
                return;
            }

            std::vector<double> lengths(population.size());
            for (size_t i = 0; i < population.size(); ++i) {
                lengths[i] = population[i].get_distance();
            }
            auto [best_length_ptr, worst_length_ptr]  =std::minmax_element(lengths.begin(), lengths.end());
            double best_length = *best_length_ptr;
            double worst_length = *worst_length_ptr;
            double average_length = std::accumulate(lengths.begin(), lengths.end(), 0.0) / lengths.size();
            
            log_file_stream << generation << "," << best_length << "," << average_length << "," << worst_length << "," << context.entropy << "," << time_per_generation << std::endl;
        }
    } logging {log_file_stream};
    
    struct {
        void operator()([[maybe_unused]]const vector<Individual>& population, Context& context, size_t generation, [[maybe_unused]]mpi::genetic_algorithm::TerminationReason reason) {
            context.final_generation = generation;
        }
    } post_process;

    // 世代交代処理
    eax::NagataGenerationChangeModel generational_step(calc_fitness_lambda, crossover_func);
    
    // GA実行オブジェクト
    mpi::GenerationalChangeModel genetic_algorithm(generational_step, update_func, logging, post_process);

    return genetic_algorithm.execute(population, context, context.current_generation);
}

Context create_context(const std::vector<Individual>& initial_population, Environment const& env) {
    Context context;
    context.env = env;

    context.set_initial_edge_counts(initial_population);
    context.random_gen = std::mt19937(env.random_seed);
    return context;
}

}