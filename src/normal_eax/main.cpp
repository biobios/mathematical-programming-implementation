#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <sstream>
#include <cmath>
#include <random>
#include <algorithm>
#include <set>
#include <map>
#include <array>
#include <chrono>
#include <unordered_set>
#include <unordered_map>
#include <list>

#include "object_pools.hpp"
#include "eax_rand.hpp"
#include "eax_n_ab.hpp"
#include "eax_block2.hpp"
#include "greedy_evaluator.hpp"
#include "entropy_evaluator.hpp"
#include "distance_preserving_evaluator.hpp"

#include "simple_ga.hpp"
#include "elitist_recombination.hpp"
#include "tsp_loader.hpp"
#include "population_initializer.hpp"
#include "individual.hpp"
#include "generational_model.hpp"
#include "context.hpp"
#include "two_opt.hpp"
#include "command_line_argument_parser.hpp"
#include <time.h>

int main(int argc, char* argv[])
{
    using namespace std;
    // TSPファイルの読み込み
    string file_name;
    // seed値
    mt19937::result_type seed = mt19937::default_seed;
    // 試行回数
    size_t trials = 1;
    // 集団サイズ
    size_t population_size = 0;
    // １度の交叉で生成する子の数
    size_t num_children = 30;
    // 評価関数の種類
    string selection_type_str = "ent"; // "greedy", "ent", or "distance"
    // 出力ファイル名
    string output_file_name = "result.md";

    // コマンドライン引数の解析
    mpi::CommandLineArgumentParser parser;
    
    mpi::ArgumentSpec file_spec(file_name);
    file_spec.add_argument_name("--file");
    file_spec.set_description("--file <filename> \t:TSP file name to load.");
    parser.add_argument(file_spec);

    mpi::ArgumentSpec ps_spec(population_size);
    ps_spec.add_argument_name("--ps");
    ps_spec.add_argument_name("--population-size");
    ps_spec.set_description("--ps <size> \t\t:Population size for the genetic algorithm.");
    parser.add_argument(ps_spec);
    
    mpi::ArgumentSpec num_children_spec(num_children);
    num_children_spec.add_argument_name("--children");
    num_children_spec.set_description("--children <number> \t:Number of children to produce per crossover (default: 30).");
    parser.add_argument(num_children_spec);
    
    mpi::ArgumentSpec trials_spec(trials);
    trials_spec.add_argument_name("--trials");
    trials_spec.set_description("--trials <number> \t:Number of trials to run.");
    parser.add_argument(trials_spec);
    
    mpi::ArgumentSpec seed_spec(seed);
    seed_spec.add_argument_name("--seed");
    seed_spec.set_description("--seed <value> \t\t:Seed value for random number generation.");
    parser.add_argument(seed_spec);

    mpi::ArgumentSpec selection_spec(selection_type_str);
    selection_spec.add_argument_name("--selection");
    selection_spec.set_description("--selection <type> \t:Selection type for the genetic algorithm. "
                                   "Options are 'greedy' for Greedy Selection, 'ent' for Entropy Selection (default), and 'distance' for Distance-preserving Selection.");
    parser.add_argument(selection_spec);
    
    mpi::ArgumentSpec output_spec(output_file_name);
    output_spec.add_argument_name("--output");
    output_spec.set_description("--output <filename> \t:Output file name (default: result.md).");
    parser.add_argument(output_spec);
    
    bool help_requested = false;
    mpi::ArgumentSpec help_spec(help_requested);
    help_spec.add_set_argument_name("--help");
    help_spec.set_description("--help \t\t\t:Show this help message.");
    parser.add_argument(help_spec);
    
    parser.parse(argc, argv);
    
    if (help_requested) {
        parser.print_help();
        return 0;
    }
    
    if (file_name.empty()) {
        cerr << "Error: TSP file name is required." << endl;
        cerr << "--file <filename> to specify the TSP file." << endl;
        return 1;
    }
    
    if (population_size == 0) {
        cerr << "Error: Population size must be greater than 0." << endl;
        cerr << "--ps <size> to specify the population size." << endl;
        return 1;
    }
    
    eax::SelectionType selection_type = eax::SelectionType::Ent;
    if (selection_type_str == "greedy") {
        selection_type = eax::SelectionType::Greedy;
    } else if (selection_type_str == "ent") {
        selection_type = eax::SelectionType::Ent;
    } else if (selection_type_str == "distance") {
        selection_type = eax::SelectionType::DistancePreserving;
    } else {
        cerr << "Error: Unknown selection type '" << selection_type_str << "'." << endl;
        cerr << "Options are 'greedy', 'ent', or 'distance'." << endl;
        return 1;
    }

    tsp::TSP tsp = tsp::TSP_Loader::load_tsp(file_name);
    cout << "TSP Name: " << tsp.name << endl;
    cout << "Distance Type: " << tsp.distance_type << endl;
    cout << "Number of Cities: " << tsp.city_count << endl;
    
    // 乱数成器(グローバル)
    mt19937 rng(seed);
    
    // neighbor_range
    size_t near_range = 50; // 近傍範囲
    // 2opt
    eax::TwoOpt two_opt(tsp.adjacency_matrix, tsp.NN_list, near_range);
    // 初期集団生成器
    tsp::PopulationInitializer population_initializer(population_size, tsp.city_count);
    
    using Individual = eax::Individual;
    
    // 1試行にかかった実時間
    vector<double> trial_times(trials, 0.0);
    // 1試行にかかったCPU時間
    vector<double> trial_cpu_times(trials, 0.0);
    // 各試行のベストの経路長
    vector<double> best_path_lengths(trials, 0.0);
    // 各試行のベストに到達した最初の世代
    vector<size_t> generation_of_best(trials, 0);
    // 各試行の最終世代
    vector<size_t> final_generations(trials, 0);
    
    for (size_t trial = 0; trial < trials; ++trial) {
        cout << "Trial " << trial + 1 << " of " << trials << endl;
        // 乱数生成器(ローカル)
        // グローバルで初期化
        mt19937::result_type local_seed = rng();
        string cache_file = "init_pop_cache_" + to_string(local_seed) + "_for_" + tsp.name + "_" + to_string(population_size) + ".txt";
        vector<vector<size_t>> initial_paths = population_initializer.initialize_population(local_seed, cache_file, [&two_opt, local_seed](vector<size_t>& path) {
            // 2-optを適用
            two_opt.apply(path, local_seed);
        });
        vector<eax::Individual> population;
        population.reserve(initial_paths.size());
        for (const auto& path : initial_paths) {
            population.emplace_back(path, tsp.adjacency_matrix);
        }

        cout << "Initial population created." << endl;
        
        using Context = eax::Context;
        // オブジェクトプール
        eax::ObjectPools object_pools(tsp.city_count);
        
        // 交叉関数
        eax::EAX_N_AB eax_n_ab(object_pools);
        eax::EAX_Block2 eax_block2(object_pools);
        auto crossover_func = [&eax_n_ab, &eax_block2](const Individual& parent1, const Individual& parent2,
                                    Context& context) {
            switch (context.eax_type) {
                case eax::EAXType::N_AB:
                    return eax_n_ab(parent1, parent2, context.num_children, context.tsp, context.random_gen, context.N_parameter);
                case eax::EAXType::Block2:
                    return eax_block2(parent1, parent2, context.num_children, context.tsp, context.random_gen);
                default:
                    throw std::runtime_error("Unknown EAX type.");
            }
        };

        // 適応度関数
        auto calc_fitness_lambda = [](const eax::CrossoverDelta& child, Context& context) {
            switch (context.selection_type) {
                case eax::SelectionType::Greedy:
                    return eax::eval::delta::Greedy()(child, context.tsp.adjacency_matrix);
                case eax::SelectionType::Ent:
                    return eax::eval::delta::Entropy()(child, context.tsp.adjacency_matrix, context.pop_edge_counts, context.population_size);
                case eax::SelectionType::DistancePreserving:
                    return eax::eval::delta::DistancePreserving()(child, context.tsp.adjacency_matrix, context.pop_edge_counts);
                default:
                    throw std::runtime_error("Unknown selection type");
            }
        };
        
        // 更新処理関数
        struct {
            mpi::genetic_algorithm::TerminationReason operator()(vector<Individual>& population, Context& context, size_t generation) {
                if (context.need_to_update_edge_counts) {
                    update_individual_and_edge_counts(population, context);
                } else {
                    update(population, context);
                }

                return continue_condition(population, context, generation);
            }
            
            void update(vector<Individual>& population, Context& context) {
                for (auto& individual : population) {
                    individual.update(context.tsp.adjacency_matrix);
                }
            }
            
            void update_individual_and_edge_counts(vector<Individual>& population, Context& context) {
                for (auto& individual : population) {
                    auto delta = individual.update(context.tsp.adjacency_matrix);
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
                
                const size_t N_child = context.num_children;
                
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
        struct {
            void operator()([[maybe_unused]]const vector<Individual>& population, [[maybe_unused]]const Context& context, size_t generation) {
                std::vector<double> lengths(population.size());
                for (size_t i = 0; i < population.size(); ++i) {
                    lengths[i] = population[i].get_distance();
                }
                double best_length = *std::min_element(lengths.begin(), lengths.end());
                double average_length = std::accumulate(lengths.begin(), lengths.end(), 0.0) / lengths.size();
                double worst_length = *std::max_element(lengths.begin(), lengths.end());
                cout << "Generation " << generation << ": Best Length = " << best_length 
                     << ", Average Length = " << average_length << ", Worst Length = " << worst_length << endl;
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

        // 環境
        Context tsp_env;
        tsp_env.tsp = tsp;
        tsp_env.population_size = population_size;
        tsp_env.N_parameter = 1;
        tsp_env.num_children = num_children;
        tsp_env.eax_type = eax::EAXType::N_AB;
        tsp_env.selection_type = selection_type;
        tsp_env.set_initial_edge_counts(population);
        tsp_env.random_gen = mt19937(local_seed);
        tsp_env.need_to_update_edge_counts = selection_type != eax::SelectionType::Greedy;
        
        cout << "Starting genetic algorithm..." << endl;
        // 計測開始
        auto start_time = chrono::high_resolution_clock::now();
        auto start_clock = clock();

        // vector<Individual> result = eax::GenerationalModel{}(population, update_func, calc_fitness_lambda, crossover_func, tsp_env, local_rng, logging);
        auto result = genetic_algorithm.execute(population, tsp_env);

        auto end_clock = clock();
        auto end_time = chrono::high_resolution_clock::now();
        trial_times[trial] = chrono::duration<double>(end_time - start_time).count();
        trial_cpu_times[trial] = static_cast<double>(end_clock - start_clock) / CLOCKS_PER_SEC;
        
        best_path_lengths[trial] = tsp_env.best_length;
        generation_of_best[trial] = tsp_env.generation_of_reached_best;
        final_generations[trial] = tsp_env.final_generation;
        
        cout << "Trial " << trial + 1 << " completed." << endl;
    }
    
    // 全試行中のベストとその解に到達した試行の数を出力する
    double best_path_length = *min_element(best_path_lengths.begin(), best_path_lengths.end());
    size_t best_path_reached_count = count_if(best_path_lengths.begin(), best_path_lengths.end(),
                                        [best_path_length](double length) { return length == best_path_length; });
    cout << "Best path length: " << best_path_length << endl;
    cout << "Number of trials that reached the best path: " << best_path_reached_count << endl;

    // ベストの平均を出力
    double average_best_path_length = accumulate(best_path_lengths.begin(), best_path_lengths.end(), 0.0) / trials;
    cout << "Average best path length: " << average_best_path_length << endl;

    // ベストに到達した最初の世代の平均を出力
    double average_generation_of_best = accumulate(generation_of_best.begin(), generation_of_best.end(), 0.0) / trials;
    cout << "Average generation of best path: " << average_generation_of_best << endl;
    
    // 最終世代の平均を出力
    double average_final_generation = accumulate(final_generations.begin(), final_generations.end(), 0.0) / trials;
    cout << "Average final generation: " << average_final_generation << endl;

    // 各試行の経過時間を出力
    cout << "Trial times (seconds): ";
    double sum_trial_times = 0;
    for (const auto& time : trial_times) {
        sum_trial_times += time;
        cout << time << " ";
    }
    cout << endl;
    
    double average_trial_time = sum_trial_times / trials;
    cout << "Average trial time: " << average_trial_time << " seconds" << endl;

    // 各試行のCPU時間を出力
    cout << "Trial CPU times (seconds): ";
    double sum_trial_cpu_times = 0;
    for (const auto& cpu_time : trial_cpu_times) {
        sum_trial_cpu_times += cpu_time;
        cout << cpu_time << " ";
    }
    cout << endl;
    
    double average_trial_cpu_time = sum_trial_cpu_times / trials;
    cout << "Average trial CPU time: " << average_trial_cpu_time << " seconds" << endl;

    eax::print_2opt_time();
    
    // 結果をMarkdown形式でファイルに追記保存
    ofstream output_file(output_file_name, ios::app);
    if (output_file.is_open()) {
        output_file << "# Genetic Algorithm Results\n\n";
        output_file << "## Summary\n";
        output_file << "- TSP Name: " << tsp.name << "\n";
        output_file << "- Distance Type: " << tsp.distance_type << "\n";
        output_file << "- Number of Cities: " << tsp.city_count << "\n";
        output_file << "- Population Size: " << population_size << "\n";
        output_file << "- Trials: " << trials << "\n";
        output_file << "- Selection Type: " << selection_type_str << "\n";
        output_file << "- Seed: " << seed << "\n\n";
        output_file << "## Results\n";
        output_file << "- Best Path Length: " << best_path_length << "\n";
        output_file << "- Number of Trials that Reached Best Path: " << best_path_reached_count << "\n";
        output_file << "- Average Best Path Length: " << average_best_path_length << "\n";
        output_file << "- Average Generation of Best Path: " << average_generation_of_best << "\n";
        output_file << "- Average Final Generation: " << average_final_generation << "\n";
        output_file << "- Average Trial Time: " << average_trial_time << " seconds\n";
        output_file << "- Average Trial CPU Time: " << average_trial_cpu_time << " seconds\n\n";
        output_file << "## Individual Trial Results\n";
        output_file << "| Trial | Best Path Length | Generation of Best | Final Generation | Trial Time (s) | Trial CPU Time (s) |\n";
        output_file << "|-------|------------------|--------------------|------------------|----------------|--------------------|\n";
        for (size_t i = 0; i < trials; ++i) {
            output_file << "| " << (i + 1) << " | " << best_path_lengths[i] << " | " << generation_of_best[i] << " | " << final_generations[i]
                        << " | " << trial_times[i] << " | " << trial_cpu_times[i] << " |\n";
        }
        output_file.close();
        cout << "Results saved to " << output_file_name << endl;
    } else {
        cerr << "Error: Unable to open output file: " << output_file_name << endl;
    }

    return 0;
}