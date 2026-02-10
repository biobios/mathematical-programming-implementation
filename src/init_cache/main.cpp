#include <string>
#include <random>

#include "command_line_argument_parser.hpp"

#include "tsp_loader.hpp"
#include "population_initializer.hpp"
#include "two_opt.hpp"

struct Arguments {
    // TSPファイルの名前
    std::string file_name;
    // 乱数のseed値
    std::mt19937::result_type seed = std::mt19937::default_seed;
    // 試行回数
    size_t trials = 1;
    // 集団サイズ
    size_t population_size = 0;
    // キャッシュディレクトリ
    std::string cache_directory = ".";
};

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
    
    mpi::ArgumentSpec trials_spec(args.trials);
    trials_spec.add_argument_name("--trials");
    trials_spec.set_description("--trials <number> \t:Number of trials to run.");
    parser.add_argument(trials_spec);
    
    mpi::ArgumentSpec seed_spec(args.seed);
    seed_spec.add_argument_name("--seed");
    seed_spec.set_description("--seed <value> \t\t:Seed value for random number generation.");
    parser.add_argument(seed_spec);

    mpi::ArgumentSpec cache_dir_spec(args.cache_directory);
    cache_dir_spec.add_argument_name("--cache-dir");
    cache_dir_spec.set_description("--cache-dir <directory> \t:Cache directory path.");
    parser.add_argument(cache_dir_spec);

    bool help_requested = false;
    mpi::ArgumentSpec help_spec(help_requested);
    help_spec.add_set_argument_name("--help");
    help_spec.set_description("--help \t\t:Display this help message.");
    parser.add_argument(help_spec);

    parser.parse(argc, argv);

    if (help_requested) {
        parser.print_help();
        return 0;
    }
    
    if (args.population_size == 0) {
        throw std::runtime_error("Population size must be greater than 0. Specify with --ps <size>.");
    }

    // 引数の表示
    cout << "TSP File: " << args.file_name << endl;
    cout << "Population Size: " << args.population_size << endl;
    cout << "Trials: " << args.trials << endl;
    cout << "Seed: " << args.seed << endl;
    cout << "Cache Directory: " << args.cache_directory << endl;

    // TSPデータの読み込み
    tsp::TSP tsp = tsp::TSP_Loader::load_tsp(args.file_name);
    // 乱数成器(グローバル)
    mt19937 rng(args.seed);
    // neighbor_range
    size_t near_range = 50; // 近傍範囲
    // 2opt
    eax::TwoOpt two_opt(tsp.adjacency_matrix, tsp.NN_list, near_range);
    // 初期集団生成器
    tsp::PopulationInitializer population_initializer(args.population_size, tsp.city_count);

    for (size_t trial = 0; trial < args.trials; ++trial) {
        // 乱数生成器(ローカル)
        // グローバルで初期化
        mt19937::result_type local_seed = rng();
        string cache_file = "init_pop_cache_" + to_string(local_seed) + "_for_" + tsp.name + "_" + to_string(args.population_size) + ".txt";

        if (args.cache_directory.ends_with('/')) {
            cache_file = args.cache_directory + cache_file;
        } else {
            cache_file = args.cache_directory + "/" + cache_file;
        }

        vector<vector<size_t>> initial_paths = population_initializer.initialize_population(local_seed, cache_file, [&two_opt, local_seed](vector<size_t>& path) {
            // 2-optを適用
            two_opt.apply(path, local_seed);
        });
    }

    return 0;
}