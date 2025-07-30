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
#include "individual.hpp"
#include "generational_model.hpp"
#include "environment.hpp"
#include "two_opt.hpp"
#include "command_line_argument_parser.hpp"
#include <time.h>

std::array<double, 2> eax::Child::calc_times;

double calc_entropy(double molecule, double denominator) {
    double ratio = molecule / denominator;
    if (ratio == 0.0) {
        return 0.0;
    }
    return -ratio * std::log2(ratio);
}

// 評価値
double eval_ent(const eax::Child& child, eax::Environment& env) {
    constexpr double epsilon = 1e-9;
    double delta_L = child.get_delta_distance(env.tsp.adjacency_matrix);
    if (delta_L >= 0.0) {
        return 0.0; // 子の距離が親より長い場合は評価値は0
    }
    double delta_H = child.get_delta_entropy(env.pop_edge_counts, env.population_size);
    
    // 多様性が増すならば
    if (delta_H >= 0) {
        return -1.0 * delta_L / epsilon;
    }

    // 多様性が減るならば
    // 減少多様性当たりの距離の減少量を評価値とする
    return delta_L / delta_H;
    
}

double eval_greedy(const eax::Child& child, const eax::Environment& env) {
    return -1.0 * child.get_delta_distance(env.tsp.adjacency_matrix);
}

void neighbor_2opt_swap(eax::Individual& ind, size_t i1, size_t i2, size_t j1, size_t j2) {
    // i2~j1の部分を逆順にする
    size_t current = i2;
    size_t end = j1;
    
    while (current != end) {
        std::swap(ind[current][0], ind[current][1]);
        current = ind[current][0]; // 次のノードへ移動
    }
    
    std::swap(ind[j1][0], ind[j1][1]);
    
    // 端を接続しなおす
    ind[i1][1] = j1;
    ind[j1][0] = i1;

    ind[i2][1] = j2;
    ind[j2][0] = i2;

}

void apply_neighbor_2opt(std::vector<size_t>& path, const tsp::TSP& tsp, size_t neighbor_size = 10) {
    using namespace std;
    
    auto& adjacency_matrix = tsp.adjacency_matrix;
    auto& NN_list = tsp.NN_list;
    
    using distance_type = std::remove_cvref_t<decltype(adjacency_matrix)>::value_type::value_type;
    const size_t n = path.size();
    eax::Individual ind(path, adjacency_matrix);
    bool improved = true;
    while (improved) {
        size_t range_start = 0;
        improved = false;
        while (!improved && range_start < n - 1) {
            size_t range_end = min(range_start + neighbor_size, n - 1);

            for (size_t i = 0; i < n && !improved; ++i) {
                size_t current_city = i;
                size_t prev_city = ind[current_city][0];
                size_t next_city = ind[current_city][1];
                
                for (size_t j = range_start; j < range_end; ++j) {
                    size_t neighbor_city = NN_list[current_city][j].second;
                    size_t neighbor_prev_city = ind[neighbor_city][0];
                    size_t neighbor_next_city = ind[neighbor_city][1];
                    
                    distance_type old_length = adjacency_matrix[current_city][prev_city];
                    distance_type new_length = adjacency_matrix[current_city][neighbor_city];
                    if (old_length > new_length) {
                        old_length += adjacency_matrix[neighbor_city][neighbor_prev_city];
                        new_length += adjacency_matrix[prev_city][neighbor_prev_city];
                        if (old_length > new_length) {
                            neighbor_2opt_swap(ind, prev_city, current_city, neighbor_prev_city, neighbor_city);
                            improved = true;
                            break;
                        }
                    }
                    
                    old_length = adjacency_matrix[current_city][next_city];
                    new_length = adjacency_matrix[current_city][neighbor_city];
                    if (old_length > new_length) {
                        old_length += adjacency_matrix[neighbor_city][neighbor_next_city];
                        new_length += adjacency_matrix[next_city][neighbor_next_city];
                        if (old_length > new_length) {
                            neighbor_2opt_swap(ind, current_city, next_city, neighbor_city, neighbor_next_city);
                            improved = true;
                            break;
                        }
                    }
                }
            }
            
            range_start += neighbor_size;
        }
    }
    
    path = ind.to_path();
}


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
    // 2-optの種類
    bool use_neighbor_2opt = true; // trueならば近傍2-opt, falseならばグローバル2-optを使用
    
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
    
    mpi::ArgumentSpec trials_spec(trials);
    trials_spec.add_argument_name("--trials");
    trials_spec.set_description("--trials <number> \t:Number of trials to run.");
    parser.add_argument(trials_spec);
    
    mpi::ArgumentSpec seed_spec(seed);
    seed_spec.add_argument_name("--seed");
    seed_spec.set_description("--seed <value> \t\t:Seed value for random number generation.");
    parser.add_argument(seed_spec);
    
    mpi::ArgumentSpec two_opt_spec(use_neighbor_2opt);
    two_opt_spec.add_set_argument_name("--neighbor-2opt");
    two_opt_spec.add_set_argument_name("--no-global-2opt");
    two_opt_spec.add_unset_argument_name("--global-2opt");
    two_opt_spec.add_unset_argument_name("--no-neighbor-2opt");
    two_opt_spec.set_description("--neighbor-2opt \t:Use neighbor 2-opt (default)."
                                 "\n--global-2opt \t\t:Use global 2-opt.");
    parser.add_argument(two_opt_spec);
    
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

    tsp::TSP tsp = tsp::TSP_Loader::load_tsp(file_name);
    cout << "TSP Name: " << tsp.name << endl;
    cout << "Distance Type: " << tsp.distance_type << endl;
    cout << "Number of Cities: " << tsp.city_count << endl;
    
    // 乱数成器(グローバル)
    mt19937 rng(seed);
    
    // neighbor_range
    size_t near_range = 50; // デフォルトの近傍範囲
    if (!use_neighbor_2opt) {
        near_range = std::numeric_limits<size_t>::max(); // グローバル2-optの場合は無制限
    }
    // 2opt
    eax::TwoOpt two_opt(tsp.adjacency_matrix, tsp.NN_list, near_range);
    // 初期集団生成器
    tsp::PopulationInitializer population_initializer(population_size, tsp.city_count, 
    [&two_opt](vector<size_t>& individual, std::mt19937::result_type seed) {
        // 2-opt法を適用
        two_opt.apply(individual, seed);
    });
    
    using Individual = eax::Individual;
    
    // 1試行にかかった実時間
    vector<double> trial_times(trials, 0.0);
    // 1試行にかかったCPU時間
    vector<double> trial_cpu_times(trials, 0.0);
    // 各試行のベストの経路長
    vector<double> best_path_lengths(trials, 0.0);
    // 各試行のベストに到達した最初の世代
    vector<size_t> generation_of_best(trials, 0);
    
    for (size_t trial = 0; trial < trials; ++trial) {
        cout << "Trial " << trial + 1 << " of " << trials << endl;
        // 乱数生成器(ローカル)
        // グローバルで初期化
        mt19937::result_type local_seed = rng();
        string cache_file = "init_pop_cache_" + to_string(local_seed) + "_for_" + file_name + "_" + to_string(population_size) + ".txt";
        vector<vector<size_t>> initial_paths = population_initializer.initialize_population(local_seed, cache_file);
        vector<eax::Individual> population;
        population.reserve(initial_paths.size());
        for (const auto& path : initial_paths) {
            population.emplace_back(path, tsp.adjacency_matrix);
        }

        cout << "Initial population created." << endl;
        
        using Env = eax::Environment;

        // 更新処理関数
        struct {
            size_t best_length = 1e18;
            size_t generation_of_reached_best = 0;
            size_t generation_of_change_to_5AB = 0;
            size_t G_devided_by_10 = 0;

            bool operator()(vector<Individual>& population, Env& env, size_t generation) {
                for (auto& individual : population) {
                    individual.update(env.tsp.adjacency_matrix).update_edge_counts(env.pop_edge_counts);
                }

                std::vector<double> lengths(population.size());
                for (size_t i = 0; i < population.size(); ++i) {
                    lengths[i] = population[i].get_distance();
                }
                
                double best_length = *std::min_element(lengths.begin(), lengths.end());
                
                if (best_length < this->best_length) {
                    this->best_length = best_length;
                    this->generation_of_reached_best = generation;
                }
                
                if (env.eax_type == eax::EAXType::N_AB && env.N_parameter == 1) {
                    if (G_devided_by_10 == 0 && generation - this->generation_of_reached_best >= 50) {
                        G_devided_by_10 = generation / 10;
                    }
                    
                    if (G_devided_by_10 > 0 && generation - this->generation_of_reached_best >= G_devided_by_10) {
                        env.N_parameter = 5;
                        this->generation_of_change_to_5AB = generation;
                        cout << "Changing N_parameter to 5 at generation " << generation << endl;
                    }
                } else if (env.eax_type == eax::EAXType::N_AB && env.N_parameter == 5) {
                    size_t last_new_record_generation = max(this->generation_of_reached_best, this->generation_of_change_to_5AB);
                    if (generation - last_new_record_generation >= 50) {
                        return false; // 5ABで50世代以上新記録が出なければ終了
                    }
                }
                
                return true;
            }
            
        } update_func;
        
        // ロガー
        struct {
            double best_length = 1e18;
            size_t generation_of_reached_best = 0;

            void operator()([[maybe_unused]]const vector<Individual>& population, [[maybe_unused]]const Env& tsp, size_t generation) {
                std::vector<double> lengths(population.size());
                for (size_t i = 0; i < population.size(); ++i) {
                    lengths[i] = population[i].get_distance();
                }
                double best_length = *std::min_element(lengths.begin(), lengths.end());
                double average_length = std::accumulate(lengths.begin(), lengths.end(), 0.0) / lengths.size();
                double worst_length = *std::max_element(lengths.begin(), lengths.end());
                cout << "Generation " << generation << ": Best Length = " << best_length 
                     << ", Average Length = " << average_length << ", Worst Length = " << worst_length << endl;
                if (best_length < this->best_length) {
                    this->best_length = best_length;
                    generation_of_reached_best = generation;
                }
            }
        } logging;

        // 適応度関数
        auto calc_fitness_lambda = [](const eax::Child& child, Env& env) {
            switch (env.selection_type) {
                case eax::SelectionType::Greedy:
                    return eval_greedy(child, env);
                case eax::SelectionType::LDL:
                    return eval_ent(child, env);
                case eax::SelectionType::Ent:
                    return eval_ent(child, env);
                default:
                    throw std::runtime_error("Unknown selection type");
            }
        };
        
        // // 乱数生成器再初期化
        mt19937 local_rng(local_seed);

        // 環境
        Env tsp_env(tsp.city_count);
        tsp_env.tsp = tsp;
        tsp_env.population_size = population_size;
        tsp_env.N_parameter = 1;
        tsp_env.eax_type = eax::EAXType::N_AB;
        // tsp_env.selection_type = eax::SelectionType::Greedy;
        tsp_env.selection_type = eax::SelectionType::Ent;
        tsp_env.set_initial_edge_counts(population);
        
        cout << "Starting genetic algorithm..." << endl;
        // 計測開始
        auto start_time = chrono::high_resolution_clock::now();
        auto start_clock = clock();

        // 世代交代モデル ElitistRecombinationを使用して、遺伝的アルゴリズムを実行
        // vector<Individual> result = mpi::genetic_algorithm::ElitistRecombination<100>(population, end_condition, calc_fitness_lambda, eax::edge_assembly_crossover, tsp, local_rng, logging);
        // vector<Individual> result = mpi::genetic_algorithm::SimpleGA(population, end_condition, calc_fitness, eax::edge_assembly_crossover, adjacency_matrix, local_rng);
        vector<Individual> result = eax::GenerationalModel<30>(population, update_func, calc_fitness_lambda, eax::edge_assembly_crossover, tsp_env, local_rng, logging);

        auto end_clock = clock();
        auto end_time = chrono::high_resolution_clock::now();
        trial_times[trial] = chrono::duration<double>(end_time - start_time).count();
        trial_cpu_times[trial] = static_cast<double>(end_clock - start_clock) / CLOCKS_PER_SEC;
        
        best_path_lengths[trial] = logging.best_length;
        generation_of_best[trial] = logging.generation_of_reached_best;
        
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

    eax::print_time();
    eax::Child::print_times();
    eax::print_2opt_time();
    return 0;
}