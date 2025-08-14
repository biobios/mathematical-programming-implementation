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
    // EAXの種類
    bool use_local_eax = true; // trueならば局所EAX, falseならばグローバルEAXを使用
    // 評価関数の種類
    bool use_greedy_selection = false; // trueならば貪欲選択, falseならばエントロピー選択を使用
    
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
    
    mpi::ArgumentSpec eax_spec(use_local_eax);
    eax_spec.add_set_argument_name("--local-eax");
    eax_spec.add_set_argument_name("--no-global-eax");
    eax_spec.add_unset_argument_name("--global-eax");
    eax_spec.add_unset_argument_name("--no-local-eax");
    eax_spec.set_description("--local-eax \t\t:Use local EAX (default)."
                             "\n--global-eax \t\t:Use global EAX.");
    parser.add_argument(eax_spec);

    mpi::ArgumentSpec selection_spec(use_greedy_selection);
    selection_spec.add_set_argument_name("--greedy-selection");
    selection_spec.add_set_argument_name("--no-ent-selection");
    selection_spec.add_unset_argument_name("--ent-selection");
    selection_spec.add_unset_argument_name("--no-greedy-selection");
    selection_spec.set_description("--greedy-selection \t:Use greedy selection (default)."
                                    "\n--ent-selection \t:Use entropy selection.");
    parser.add_argument(selection_spec);
    
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
    
    for (size_t trial = 0; trial < trials; ++trial) {
        cout << "Trial " << trial + 1 << " of " << trials << endl;
        // 乱数生成器(ローカル)
        // グローバルで初期化
        mt19937::result_type local_seed = rng();
        string cache_file = "init_pop_cache_" + to_string(local_seed) + "_for_" + file_name + "_" + to_string(population_size) + ".txt";
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
        
        using Env = eax::Environment;

        // 更新処理関数
        struct {
            size_t best_length = 1e18;
            size_t generation_of_reached_best = 0;
            size_t generation_of_change_to_5AB = 0;
            size_t G_devided_by_10 = 0;
            eax::EAXType eax_type;
            eax::SelectionType selection_type;

            bool operator()(vector<Individual>& population, Env& env, size_t generation) {
                if (selection_type == eax::SelectionType::Greedy) {
                    update_greedy(population, env);
                } else {
                    update_entropy(population, env);
                }

                if (eax_type == eax::EAXType::Rand) {
                    return continue_condition_global(population);
                } else {
                    return continue_condition_local(population, env, generation);
                }
            }
            
            void update_greedy(vector<Individual>& population, Env& env) {
                for (auto& individual : population) {
                    individual.update(env.tsp.adjacency_matrix);
                }
            }
            
            void update_entropy(vector<Individual>& population, Env& env) {
                for (auto& individual : population) {
                    individual.update(env.tsp.adjacency_matrix).update_edge_counts(env.pop_edge_counts);
                }
            }
            
            bool continue_condition_global(const vector<Individual>& population) {
                double best_length = std::numeric_limits<double>::max();
                double average_length = 0.0;
                for (const auto& individual : population) {
                    double length = individual.get_distance();
                    best_length = std::min(best_length, length);
                    average_length += length;
                }
                average_length /= population.size();
                return (average_length - best_length) > 0.1;
            }

            bool continue_condition_local(const vector<Individual>& population, Env& env, size_t generation) {
                double best_length = std::numeric_limits<double>::max();
                for (size_t i = 0; i < population.size(); ++i) {
                    double length = population[i].get_distance();
                    best_length = std::min(best_length, length);
                }
                
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
        } update_func {
            .eax_type = use_local_eax ? eax::EAXType::N_AB : eax::EAXType::Rand,
            .selection_type = use_greedy_selection ? eax::SelectionType::Greedy : eax::SelectionType::Ent
        };
        
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
        if (use_local_eax) {
            tsp_env.eax_type = eax::EAXType::N_AB;
        } else {
            tsp_env.eax_type = eax::EAXType::Rand;
        }
        if (use_greedy_selection) {
            tsp_env.selection_type = eax::SelectionType::Greedy;
        } else {
            tsp_env.selection_type = eax::SelectionType::Ent;
        }
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