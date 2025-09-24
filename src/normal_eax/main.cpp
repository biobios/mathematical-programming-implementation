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
#include "ga.hpp"
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
    // タイムアウト時間(秒)
    size_t timeout_seconds = 60 * 60 * 24 * 365; // 実質的に無制限
    // 中断時の状態を保存するファイルの名前
    string checkpoint_save_file_name = "checkpoint.dat";
    // チェックポイントのファイル名 (指定されれば読み込む)
    string checkpoint_load_file_name;

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
    
    mpi::ArgumentSpec timeout_spec(timeout_seconds);
    timeout_spec.add_argument_name("--timeout");
    timeout_spec.set_description("--timeout <seconds> \t:Timeout duration in seconds.");
    parser.add_argument(timeout_spec);
    
    mpi::ArgumentSpec checkpoint_save_spec(checkpoint_save_file_name);
    checkpoint_save_spec.add_argument_name("--checkpoint-save");
    checkpoint_save_spec.set_description("--checkpoint-save <filename> \t:File name to save checkpoint state (default: checkpoint.dat).");
    parser.add_argument(checkpoint_save_spec);

    mpi::ArgumentSpec checkpoint_load_spec(checkpoint_load_file_name);
    checkpoint_load_spec.add_argument_name("--checkpoint-load");
    checkpoint_load_spec.set_description("--checkpoint-load <filename> \t:File name to load checkpoint state.");
    parser.add_argument(checkpoint_load_spec);
    
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
    
    // 現在の時刻を取得し、タイムアウトする時刻を計算
    auto current_time = chrono::system_clock::now();
    auto timeout_time = current_time + chrono::seconds(timeout_seconds);

    if (!checkpoint_load_file_name.empty()) {
        if (seed != mt19937::default_seed) {
            cerr << "Warning: --seed is ignored when --checkpoint-load is specified." << endl;
        }
        if (population_size != 0) {
            cerr << "Warning: --ps is ignored when --checkpoint-load is specified." << endl;
        }
        if (num_children != 30) {
            cerr << "Warning: --children is ignored when --checkpoint-load is specified." << endl;
        }
        if (selection_type_str != "ent") {
            cerr << "Warning: --selection is ignored when --checkpoint-load is specified." << endl;
        }
        if (trials != 1) {
            cerr << "Warning: --trials is ignored when --checkpoint-load is specified." << endl;
        }

        // TSPファイルを読み込む
        ifstream tsp_file(file_name);
        if (!tsp_file.is_open()) {
            cerr << "Error: Failed to open TSP file: " << file_name << endl;
            return 1;
        }
        tsp::TSP tsp = tsp::TSP_Loader::load_tsp(file_name);
        tsp_file.close();

        // チェックポイントファイルを読み込む
        ifstream checkpoint_file(checkpoint_load_file_name);
        if (!checkpoint_file.is_open()) {
            cerr << "Error: Failed to open checkpoint file: " << checkpoint_load_file_name << endl;
            return 1;
        }
        eax::Context context = eax::deserialize_context(checkpoint_file, std::move(tsp));
        vector<eax::Individual> population = eax::deserialize_population(checkpoint_file);
        checkpoint_file.close();

        // 遺伝的アルゴリズムの実行
        cout << "Resuming from checkpoint..." << endl;
        auto result = eax::execute_ga(population, context, timeout_time);
        
        auto& [termination_reason, result_population] = result;
        if (termination_reason == mpi::genetic_algorithm::TerminationReason::TimeLimit) {
            ofstream checkpoint_out(checkpoint_save_file_name);
            if (!checkpoint_out.is_open()) {
                cerr << "Error: Failed to open checkpoint save file: " << checkpoint_save_file_name << endl;
                return 1;
            }
            eax::serialize_context(context, checkpoint_out);
            eax::serialize_population(result_population, checkpoint_out);
            checkpoint_out.close();
            cout << "Checkpoint saved to " << checkpoint_save_file_name << endl;
        } else {
            // 結果を出力
            ofstream result_file(output_file_name, ios::app);

            // result_fileが空であればヘッダーを書き込む
            result_file.seekp(0, ios::end);
            if (result_file.tellp() == 0) {
                result_file << "# EAX Genetic Algorithm Results" << endl;
                result_file << endl;
                result_file << "| TSP Name | Population Size | Selection Type | Children per Crossover | Seed | Best Length | Generation Reached Best | Total Generations | Time (s) |" << endl;
                result_file << "|----------|-----------------|----------------|-----------------------|------|-------------|------------------------|-------------------|----------|" << endl;
            }
            
            result_file << "| " << context.env.tsp.name << " | " << context.env.population_size << " | ";
            switch (context.env.selection_type) {
                case eax::SelectionType::Greedy:
                    result_file << "greedy";
                    break;
                case eax::SelectionType::Ent:
                    result_file << "ent";
                    break;
                case eax::SelectionType::DistancePreserving:
                    result_file << "distance";
                    break;
                default:
                    result_file << "unknown";
                    break;
            }
            result_file << " | " << context.env.num_children << " | " << context.env.random_seed << " | "
                        << context.best_length << " | " << context.generation_of_reached_best << " | "
                        << context.final_generation << " | " << context.elapsed_time << " |" << endl;
            result_file.close();
            cout << "Result saved to " << output_file_name << endl;
        }
    } else {
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

            // 環境
            eax::Environment ga_env{tsp, population_size, num_children, selection_type, local_seed};
            eax::Context ga_context = eax::create_context(population, ga_env);
            
            cout << "Starting genetic algorithm..." << endl;
            // 計測開始
            auto result = eax::execute_ga(population, ga_context, timeout_time);
            auto& [termination_reason, result_population] = result;
            
            if (termination_reason == mpi::genetic_algorithm::TerminationReason::TimeLimit) {
                ofstream checkpoint_out(checkpoint_save_file_name);
                if (!checkpoint_out.is_open()) {
                    cerr << "Error: Failed to open checkpoint save file: " << checkpoint_save_file_name << endl;
                    return 1;
                }
                eax::serialize_context(ga_context, checkpoint_out);
                eax::serialize_population(result_population, checkpoint_out);
                checkpoint_out.close();
                cout << "Checkpoint saved to " << checkpoint_save_file_name << endl;
            } else {
                // 結果を出力
                ofstream result_file(output_file_name, ios::app);

                // result_fileが空であればヘッダーを書き込む
                result_file.seekp(0, ios::end);
                if (result_file.tellp() == 0) {
                    result_file << "# EAX Genetic Algorithm Results" << endl;
                    result_file << endl;
                    result_file << "| TSP Name | Population Size | Selection Type | Children per Crossover | Seed | Best Length | Generation Reached Best | Total Generations | Time (s) |" << endl;
                    result_file << "|----------|-----------------|----------------|-----------------------|------|-------------|------------------------|-------------------|----------|" << endl;
                }
                
                result_file << "| " << tsp.name << " | " << population_size << " | " << selection_type_str << " | "
                            << num_children << " | " << local_seed << " | " << ga_context.best_length << " | " << ga_context.generation_of_reached_best << " | "
                            << ga_context.final_generation << " | " << ga_context.elapsed_time << " |" << endl;
                result_file.close();
                cout << "Result saved to " << output_file_name << endl;
            }
            
            cout << "Trial " << trial + 1 << " completed." << endl;
        }
    }

    return 0;
}