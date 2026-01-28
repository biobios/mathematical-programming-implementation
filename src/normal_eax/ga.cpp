#include "ga.hpp"

#include <iostream>
#include <fstream>

#include "genetic_algorithm.hpp"
#include "generational_change_model.hpp"

#include "object_pools.hpp"
#include "eax_n_ab.hpp"
#include "eax_block2.hpp"
#include "greedy_evaluator.hpp"
#include "entropy_evaluator.hpp"
#include "distance_preserving_evaluator.hpp"
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
    auto crossover_func = [&eax_n_ab, &eax_block2](const Individual& parent1, const Individual& parent2,
                                Context& context) {
        auto& env = context.env;
        switch (context.eax_type) {
            case eax::EAXType::One_AB:
                return eax_n_ab(parent1, parent2, env.num_children, env.tsp, context.random_gen, 1);
            case eax::EAXType::Block2:
                return eax_block2(parent1, parent2, env.num_children, env.tsp, context.random_gen);
            default:
                throw std::runtime_error("Unknown EAX type.");
        }
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

                context.entropy += eax::calc_delta_entropy(delta, context.pop_edge_counts, context.env.population_size);
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
            
            if (context.stage == Context::GA_Stage::Stage1) {
                if (context.G_devided_by_10 == 0 && context.stagnation_generations >= (1500 / N_child)) {
                    context.G_devided_by_10 = generation / 10;
                } else if (context.G_devided_by_10 > 0 && context.stagnation_generations >= context.G_devided_by_10) {
                    context.stage = Context::GA_Stage::Stage2;
                    context.eax_type = eax::EAXType::Block2;
                    context.stagnation_generations = 0;
                    context.generation_of_transition_to_stage2 = generation;
                    context.G_devided_by_10 = 0;
                }
            } else {
                if (context.G_devided_by_10 == 0 && context.stagnation_generations >= (1500 / N_child)) {
                    context.G_devided_by_10 = (generation - context.generation_of_transition_to_stage2) / 10;
                } else if (context.G_devided_by_10 > 0 && context.stagnation_generations >= context.G_devided_by_10) {
                    return mpi::genetic_algorithm::TerminationReason::Stagnation; // 停滞条件
                }
            }
            
            return mpi::genetic_algorithm::TerminationReason::NotTerminated;
        }
    } update_func;
    
    // ロガー
    std::ofstream log_file;
    if (!log_file_name.empty()) {
        log_file.open(log_file_name, std::ios::out);
        if (!log_file.is_open()) {
            throw std::runtime_error("Failed to open log file: " + log_file_name);
        }
    }
    struct {
        std::ofstream& out;
        void operator()([[maybe_unused]]const vector<Individual>& population, Context& context, size_t generation) {
            if (context.start_time.time_since_epoch().count() == 0) {
                // 計測開始時刻が未設定なら、現在時刻を設定
                context.start_time = std::chrono::system_clock::now();
            } else {
                auto now = std::chrono::system_clock::now();
                context.elapsed_time += std::chrono::duration<double>(now - context.start_time).count();
                context.start_time = now;
            }
            if (!out.is_open()) return;

            std::vector<double> lengths(population.size());
            for (size_t i = 0; i < population.size(); ++i) {
                lengths[i] = population[i].get_distance();
            }
            double best_length = *std::min_element(lengths.begin(), lengths.end());
            double average_length = std::accumulate(lengths.begin(), lengths.end(), 0.0) / lengths.size();
            double worst_length = *std::max_element(lengths.begin(), lengths.end());
            out << generation << ","
                << best_length << ","
                << average_length << ","
                << worst_length << ","
                << context.entropy
                << std::endl;
        }
    } logging {log_file};
    
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

    context.eax_type = EAXType::One_AB;
    context.set_initial_edge_counts(initial_population);
    context.random_gen = std::mt19937(env.random_seed);
    return context;
}
    
}