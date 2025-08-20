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

#include <time.h>

#include "simple_ga.hpp"
#include "elitist_recombination.hpp"
#include "command_line_argument_parser.hpp"

#include "tsp_loader.hpp"
#include "population_initializer.hpp"
#include "two_opt.hpp"
#include "object_pools.hpp"
#include "eax_rand.hpp"

double calc_fitness(const std::vector<size_t>& path, const std::vector<std::vector<int64_t>>& adjacency_matrix){
    double distance = 0.0;
    for (size_t i = 0; i < path.size() - 1; ++i) {
        distance += adjacency_matrix[path[i]][path[i + 1]];
    }
    // 最後の都市から最初の都市への距離を加算
    distance += adjacency_matrix[path.back()][path.front()];
    return 1.0 / distance;
}

double calc_fitness(const std::vector<std::array<size_t, 2>>& adjacency_list, const std::vector<std::vector<int64_t>>& adjacency_matrix) {
    double distance = 0.0;
    for (size_t i = 0; i < adjacency_list.size(); ++i) {
        size_t next1 = adjacency_list[i][0];
        size_t next2 = adjacency_list[i][1];
        distance += adjacency_matrix[i][next1];
        distance += adjacency_matrix[i][next2];
    }
    distance /= 2.0;
    return 1.0 / distance;
}

int main(int argc, char* argv[])
{
    using namespace std;
    // TSPファイルの読み込み
    string file_name = "att532.tsp";

    // seed値
    mt19937::result_type seed = mt19937::default_seed;
    // 試行回数
    size_t trials = 1;
    // 世代数
    size_t generations = 300;
    // 集団サイズ
    size_t population_size = 0;
    
    mpi::CommandLineArgumentParser parser;
    
    mpi::ArgumentSpec file_spec(file_name);
    file_spec.add_argument_name("--file");
    file_spec.set_description("--file <filename> \t: Specify the TSP file to load.");
    parser.add_argument(std::move(file_spec));

    mpi::ArgumentSpec ps_spec(population_size);
    ps_spec.add_argument_name("--ps");
    ps_spec.set_description("--ps <size> \t: Specify the population size.");
    parser.add_argument(std::move(ps_spec));
    
    mpi::ArgumentSpec trials_spec(trials);
    trials_spec.add_argument_name("--trials");
    trials_spec.set_description("--trials <number> \t: Specify the number of trials.");
    parser.add_argument(std::move(trials_spec));

    mpi::ArgumentSpec generations_spec(generations);
    generations_spec.add_argument_name("--generations");
    generations_spec.set_description("--generations <number> \t: Specify the number of generations.");
    parser.add_argument(std::move(generations_spec));
    
    mpi::ArgumentSpec seed_spec(seed);
    seed_spec.add_argument_name("--seed");
    seed_spec.set_description("--seed <value> \t: Specify the random seed value.");
    parser.add_argument(std::move(seed_spec));

    bool help_requested = false;
    mpi::ArgumentSpec help_spec(help_requested);
    help_spec.add_set_argument_name("--help");
    help_spec.set_description("--help \t: Show this help message.");
    parser.add_argument(std::move(help_spec));

    parser.parse(argc, argv);
    
    if (help_requested) {
        parser.print_help();
        return 0;
    }
    
    tsp::TSP tsp = tsp::TSP_Loader::load_tsp(file_name);
    cout << "TSP Name: " << tsp.name << endl;
    cout << "Distance Type: " << tsp.distance_type << endl;
    cout << "Number of Cities: " << tsp.city_count << endl;
    
    // 2 optを適用するためのオブジェクト
    eax::TwoOpt two_opt(tsp.adjacency_matrix, tsp.NN_list, std::numeric_limits<size_t>::max());
    // 乱数生成器(グローバル)
    mt19937 rng(seed);
    
    if (tsp.name == "att532" && population_size == 0) {
        population_size = 250;
    } else if(tsp.name == "rat575" && population_size == 0) {
        population_size = 300;
    } else if(population_size == 0) {
        cerr << "Population size must be specified with --ps option." << endl;
        return 1;
    }

    // 初期集団生成器
    tsp::PopulationInitializer population_initializer(population_size, tsp.city_count);
    
    using Individual = vector<array<size_t, 2>>;
    
    // 1試行にかかった時間
    vector<double> trial_times(trials, 0.0);
    // 1試行にかかったCPU時間
    vector<double> cpu_trial_times(trials, 0.0);
    // 各試行のベストの経路長
    vector<double> best_path_lengths(trials, 0.0);
    // 各試行のベストに到達した最初の世代
    vector<size_t> generation_of_best(trials, 0);
    
    for (size_t trial = 0; trial < trials; ++trial) {
        cout << "Trial " << trial + 1 << " of " << trials << endl;

        mt19937::result_type local_seed = rng();
        string cache_file = "init_pop_cache_" + to_string(local_seed) + "_for_" + file_name + "_" + to_string(population_size) + ".txt";
        vector<vector<size_t>> paths = population_initializer.initialize_population(local_seed, cache_file, [&two_opt, local_seed](vector<size_t>& path) {
            // 2-optを適用
            two_opt.apply(path, local_seed);
        });
        
        vector<Individual> population;
        population.reserve(population_size);
        for (const auto& path : paths) {
            Individual individual;
            individual.resize(path.size());
            for (size_t i = 1; i < path.size() - 1; ++i) {
                individual[path[i]] = {path[i - 1], path[i + 1]};
            }
            individual[path[0]] = {path.back(), path[1]};
            individual[path.back()] = {path[path.size() - 2], path[0]};
            population.push_back(std::move(individual));
        }
        cout << "Initial population created." << endl;
        
        using Env = tsp::TSP;
        
        eax::ObjectPools object_pools(tsp.city_count);
        eax::EAX_Rand eax_rand(object_pools);
        
        auto crossover = [&eax_rand](const Individual& parent1, const Individual& parent2, size_t children_size,
                                            Env& env, mt19937& rng) {
            auto deltas = eax_rand(parent1, parent2, children_size, env, rng);
            std::vector<Individual> children(deltas.size(), parent1);
            for (size_t i = 0; i < deltas.size(); ++i) {
                deltas[i].apply_to(children[i]);
            }
            return children;
        };

        // 終了判定関数
        // 世代数に達するか、収束するまで実行
        struct {
            size_t max_generations;

            bool operator()([[maybe_unused]]const vector<Individual>& population, const vector<double>& fitness_values, [[maybe_unused]]const Env& tsp, size_t generation) {

                double max_fitness = 0.0;
                double min_fitness = std::numeric_limits<double>::max();
                for (const auto& fitness : fitness_values) {
                    max_fitness = std::max(max_fitness, fitness);
                    min_fitness = std::min(min_fitness, fitness);
                }
                
                // 終了条件を満たすかどうかを判定
                return generation > max_generations || max_fitness == min_fitness;
            }
        } end_condition = {.max_generations = generations};
        
        // ロガー
        struct {
            double best_fitness = 0.0;
            size_t generation_of_reached_best = 0;

            void operator()([[maybe_unused]]const vector<Individual>& population, const vector<double>& fitness_values, [[maybe_unused]]const Env& tsp, size_t generation) {
                double max_fitness = *max_element(fitness_values.begin(), fitness_values.end());
                if (max_fitness > best_fitness) {
                    best_fitness = max_fitness;
                    generation_of_reached_best = generation;
                }
            }
        } logging;

        // 適応度関数
        auto calc_fitness_lambda = [](const Individual& individual, const Env& env) {
            return calc_fitness(individual, env.adjacency_matrix);
        };
        
        // 乱数生成器初期化
        mt19937 local_rng(local_seed);
        
        // 環境情報の設定
        Env& env = tsp;       

        // 計測開始
        auto start_time = chrono::high_resolution_clock::now();
        auto start_cpu_time = clock();

        // 世代交代モデル ElitistRecombinationを使用して、遺伝的アルゴリズムを実行
        vector<Individual> result = mpi::genetic_algorithm::ElitistRecombination<100>(population, end_condition, calc_fitness_lambda, crossover, env, local_rng, logging);
        // vector<Individual> result = mpi::genetic_algorithm::SimpleGA(population, end_condition, calc_fitness, eax::edge_assembly_crossover, adjacency_matrix, local_rng);
        
        auto end_cpu_time = clock();
        auto end_time = chrono::high_resolution_clock::now();
        cpu_trial_times[trial] = static_cast<double>(end_cpu_time - start_cpu_time) / CLOCKS_PER_SEC;
        trial_times[trial] = chrono::duration<double>(end_time - start_time).count();
        
        best_path_lengths[trial] = 1.0 / logging.best_fitness;
        generation_of_best[trial] = logging.generation_of_reached_best;
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
    double sum_cpu_trial_times = 0;
    for (const auto& cpu_time : cpu_trial_times) {
        sum_cpu_trial_times += cpu_time;
        cout << cpu_time << " ";
    }
    cout << endl;

    double average_cpu_trial_time = sum_cpu_trial_times / trials;
    cout << "Average CPU trial time: " << average_cpu_trial_time << " seconds" << endl;

    return 0;
}