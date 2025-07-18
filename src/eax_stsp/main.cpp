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

#include "simple_ga.hpp"
#include "elitist_recombination.hpp"
#include "tsp_loader.hpp"
#include "population_initializer.hpp"
#include "eax.hpp"

double calc_fitness(const std::vector<size_t>& path, const std::vector<std::vector<int64_t>>& adjacency_matrix){
    double distance = 0.0;
    for (size_t i = 0; i < path.size() - 1; ++i) {
        distance += adjacency_matrix[path[i]][path[i + 1]];
    }
    // 最後の都市から最初の都市への距離を加算
    distance += adjacency_matrix[path.back()][path.front()];
    return 1.0 / distance;
}

void two_opt_swap(std::vector<size_t>& path, size_t i, size_t j) {
    // iとjの間の部分を逆順にする
    using namespace std;
    std::reverse(path.begin() + i, path.begin() + j + 1);
}

void apply_2opt(std::vector<size_t>& path, const std::vector<std::vector<int64_t>>& adjacency_matrix) {
    using namespace std;
    using distance_type = std::remove_cvref_t<decltype(adjacency_matrix)>::value_type::value_type;
    size_t n = path.size();
    bool improved = true;
    while (improved) {
        improved = false;
        for (size_t i = 0; i < path.size() - 1 && !improved; ++i) {
            for (size_t j = i + 1; j < path.size(); ++j) {
                distance_type old_length = adjacency_matrix[path[i]][path[i + 1]] + adjacency_matrix[path[j]][path[(j + 1) % n]];
                distance_type new_length = adjacency_matrix[path[i]][path[j]] + adjacency_matrix[path[i + 1]][path[(j + 1) % n]];
                
                if (new_length < old_length) {
                    two_opt_swap(path, i + 1, j);
                    improved = true;
                    break;
                }
            }
        }
    }
}


int main(int argc, char* argv[])
{
    using namespace std;
    // TSPファイルの読み込み
    // string file_name = "rat575.tsp";
    string file_name = "att532.tsp";
    if (argc > 1) {
        file_name = argv[1];
    }
    tsp::TSP tsp = tsp::TSP_Loader::load_tsp(file_name);
    cout << "TSP Name: " << tsp.name << endl;
    cout << "Distance Type: " << tsp.distance_type << endl;
    cout << "Number of Cities: " << tsp.city_count << endl;
    
    // 試行回数
    constexpr size_t trials = 3;
    // 世代数
    constexpr size_t generations = 300;
    // 乱数生成器(グローバル)
    // mt19937 rng;
    mt19937 rng(545404204);
    // 集団サイズ
    size_t population_size = 0;
    if (tsp.name == "rat575") {
        population_size = 300;
    } else if (tsp.name == "att532") {
        population_size = 250;
    } else {
        cerr << "Unsupported TSP file: " << tsp.name << endl;
        return 1;
    }
    // 初期集団生成器
    tsp::PopulationInitializer population_initializer(population_size, tsp.city_count, 
    [&tsp](vector<size_t>& individual) {
        // 2-opt法を適用
        apply_2opt(individual, tsp.adjacency_matrix);
    });
    
    using Individual = vector<size_t>;
    
    // 1試行にかかった時間
    vector<double> trial_times(trials, 0.0);
    // 各試行のベストの経路長
    vector<double> best_path_lengths(trials, 0.0);
    // 各試行のベストに到達した最初の世代
    vector<size_t> generation_of_best(trials, 0);
    
    for (size_t trial = 0; trial < trials; ++trial) {
        cout << "Trial " << trial + 1 << " of " << trials << endl;
        // 乱数生成器(ローカル)
        // グローバルで初期化
        mt19937::result_type seed = rng();
        vector<Individual> population = population_initializer.initialize_population(seed, "initial_population_cache_" + to_string(seed) + "_for_" + file_name);
        cout << "Initial population created." << endl;
        
        using Env = tsp::TSP;

        // 終了判定関数
        // 世代数に達するか、収束するまで実行
        struct {
            size_t max_generations = generations;

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
        } end_condition;
        
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
        auto calc_fitness_lambda = [](const Individual& individual, const Env& tsp) {
            return calc_fitness(individual, tsp.adjacency_matrix);
        };
        
        // // 乱数生成器再初期化
        mt19937 local_rng(seed);
        
        // 計測開始
        auto start_time = chrono::high_resolution_clock::now();

        // 世代交代モデル ElitistRecombinationを使用して、遺伝的アルゴリズムを実行
        vector<Individual> result = mpi::genetic_algorithm::ElitistRecombination<100>(population, end_condition, calc_fitness_lambda, eax::edge_assembly_crossover, tsp, local_rng, logging);
        // vector<Individual> result = mpi::genetic_algorithm::SimpleGA(population, end_condition, calc_fitness, eax::edge_assembly_crossover, adjacency_matrix, local_rng);
        
        auto end_time = chrono::high_resolution_clock::now();
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

    eax::print_time();
    return 0;
}