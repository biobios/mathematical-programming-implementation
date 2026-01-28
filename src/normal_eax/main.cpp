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
#include "context.hpp"
#include "ga.hpp"
#include "two_opt.hpp"
#include "command_line_argument_parser.hpp"
#include <time.h>

struct Arguments {
    // TSPファイルの名前
    std::string file_name;
    // 乱数のseed値
    std::mt19937::result_type seed = std::mt19937::default_seed;
    // 試行回数
    size_t trials = 1;
    // 集団サイズ
    size_t population_size = 0;
    // １度の交叉で生成する子の数
    size_t num_children = 30;
    // 評価関数の種類
    std::string selection_type_str = "ent"; // "greedy", "ent", or "distance"
    // 出力ファイル名
    std::string output_file_name = "result.md";
    // logファイル名
    std::string log_file_name = "";
};

void print_result(const eax::Context& context, std::ostream& os)
{
    os.seekp(0, std::ios::end);
    if (os.tellp() == 0) {
        os << "# EAX Genetic Algorithm Results" << std::endl;
        os << std::endl;
        os << "| TSP Name | Population Size | Selection Type | Children per Crossover | Seed | Best Length | Generation Reached Best | Total Generations | Time (s) |" << std::endl;
        os << "|----------|-----------------|----------------|-----------------------|------|-------------|------------------------|-------------------|----------|" << std::endl;
    }
    
    os << "| " << context.env.tsp.name << " | " << context.env.population_size << " | "; 
    switch (context.env.selection_type) {
        case eax::SelectionType::Greedy:
            os << "greedy";
            break;
        case eax::SelectionType::Ent:
            os << "ent";
            break;
        case eax::SelectionType::DistancePreserving:
            os << "distance";
            break;
        default:
            os << "unknown";
            break;
    }
    os << " | " << context.env.num_children << " | " << context.env.random_seed << " | " << context.best_length << " | " << context.generation_of_reached_best << " | "
                << context.final_generation << " | " << context.elapsed_time << " |" << std::endl;

}

// 通常実行
void execute_normal(const Arguments& args)
{
    using namespace std;
    if (args.population_size == 0) {
        throw std::runtime_error("Population size must be greater than 0. Specify with --ps <size>.");
    }

    eax::SelectionType selection_type = eax::SelectionType::Ent;
    if (args.selection_type_str == "greedy") {
        selection_type = eax::SelectionType::Greedy;
    } else if (args.selection_type_str == "ent") {
        selection_type = eax::SelectionType::Ent;
    } else if (args.selection_type_str == "distance") {
        selection_type = eax::SelectionType::DistancePreserving;
    } else {
        throw std::runtime_error("Unknown selection type '" + args.selection_type_str + "'. Options are 'greedy', 'ent', or 'distance'.");
    }

    tsp::TSP tsp = tsp::TSP_Loader::load_tsp(args.file_name);
    cout << "TSP Name: " << tsp.name << endl;
    cout << "Distance Type: " << tsp.distance_type << endl;
    cout << "Number of Cities: " << tsp.city_count << endl;
    
    // 乱数成器(グローバル)
    mt19937 rng(args.seed);
    
    // neighbor_range
    size_t near_range = 50; // 近傍範囲
    // 2opt
    eax::TwoOpt two_opt(tsp.adjacency_matrix, tsp.NN_list, near_range);
    // 初期集団生成器
    tsp::PopulationInitializer population_initializer(args.population_size, tsp.city_count);
    
    for (size_t trial = 0; trial < args.trials; ++trial) {
        cout << "Trial " << trial + 1 << " of " << args.trials << endl;
        // 乱数生成器(ローカル)
        // グローバルで初期化
        mt19937::result_type local_seed = rng();
        string cache_file = "init_pop_cache_" + to_string(local_seed) + "_for_" + tsp.name + "_" + to_string(args.population_size) + ".txt";
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
        eax::Environment ga_env{tsp, args.population_size, args.num_children, selection_type, local_seed};
        eax::Context ga_context = eax::create_context(population, ga_env);
        
        cout << "Starting genetic algorithm..." << endl;
        // 計測開始
        eax::execute_ga(population, ga_context, args.log_file_name);
        
        // 結果を出力
        ofstream result_file(args.output_file_name, ios::app);
        if (!result_file.is_open()) {
            throw std::runtime_error("Failed to open result file: " + args.output_file_name);
        }

        print_result(ga_context, result_file);
        result_file.close();
        cout << "Result saved to " << args.output_file_name << endl;
        
        cout << "Trial " << trial + 1 << " completed." << endl;
    }
}

int main(int argc, char* argv[])
{
    using namespace std;
    Arguments args;
    // コマンドライン引数の解析
    mpi::CommandLineArgumentParser parser;
    
    mpi::ArgumentSpec file_spec(args.file_name);
    file_spec.add_argument_name("--file");
    file_spec.set_description("--file <filename> \t:TSP file name to load.");
    parser.add_argument(file_spec);

    mpi::ArgumentSpec ps_spec(args.population_size);
    ps_spec.add_argument_name("--ps");
    ps_spec.add_argument_name("--population-size");
    ps_spec.set_description("--ps <size> \t\t:Population size for the genetic algorithm.");
    parser.add_argument(ps_spec);
    
    mpi::ArgumentSpec num_children_spec(args.num_children);
    num_children_spec.add_argument_name("--children");
    num_children_spec.set_description("--children <number> \t:Number of children to produce per crossover (default: 30).");
    parser.add_argument(num_children_spec);
    
    mpi::ArgumentSpec trials_spec(args.trials);
    trials_spec.add_argument_name("--trials");
    trials_spec.set_description("--trials <number> \t:Number of trials to run.");
    parser.add_argument(trials_spec);
    
    mpi::ArgumentSpec seed_spec(args.seed);
    seed_spec.add_argument_name("--seed");
    seed_spec.set_description("--seed <value> \t\t:Seed value for random number generation.");
    parser.add_argument(seed_spec);

    mpi::ArgumentSpec selection_spec(args.selection_type_str);
    selection_spec.add_argument_name("--selection");
    selection_spec.set_description("--selection <type> \t:Selection type for the genetic algorithm. "
                                   "Options are 'greedy' for Greedy Selection, 'ent' for Entropy Selection (default), and 'distance' for Distance-preserving Selection.");
    parser.add_argument(selection_spec);
    
    mpi::ArgumentSpec output_spec(args.output_file_name);
    output_spec.add_argument_name("--output");
    output_spec.set_description("--output <filename> \t:Output file name (default: result.md).");
    parser.add_argument(output_spec);

    mpi::ArgumentSpec log_spec(args.log_file_name);
    log_spec.add_argument_name("--log");
    log_spec.set_description("--log <filename> \t:Log file name.");
    parser.add_argument(log_spec);
    
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
    
    if (args.file_name.empty()) {
        cerr << "Error: TSP file name is required." << endl;
        cerr << "--file <filename> to specify the TSP file." << endl;
        return 1;
    }

    execute_normal(args);

    return 0;
}