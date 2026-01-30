#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>

#include "tsp_loader.hpp"
#include "population_initializer.hpp"
#include "buffered_individual.hpp"
#include "eax_rand.hpp"
#include "object_pools.hpp"

int main() {
    using namespace std;
    
    // TSPファイルの読み込み
    tsp::TSP tsp = tsp::TSP_Loader::load_tsp("../../data/sample_eax/att532.tsp");
    cout << "TSP Name: " << tsp.name << endl;
    cout << "City Count: " << tsp.city_count << endl;
    
    // 乱数生成器
    mt19937 rng(42);
    
    // 初期集団のサイズ
    size_t population_size = 100;
    
    // 初期集団の生成
    tsp::PopulationInitializer population_initializer(population_size, tsp.city_count);
    mt19937::result_type seed = rng();
    vector<vector<size_t>> initial_paths = population_initializer.initialize_population(seed, "init_pop_cache.txt");
    
    using Individual = eax::BufferedIndividual;

    // BufferedIndividualに変換
    vector<Individual> population;
    population.reserve(initial_paths.size());
    for (const auto& path : initial_paths) {
        population.emplace_back(path, tsp.adjacency_matrix);
    }
    
    cout << "Initial population created." << endl;
    cout << "Initial best distance: " << min_element(population.begin(), population.end(),
        [](const auto& a, const auto& b) { return a.get_distance() < b.get_distance(); })->get_distance() << endl;
    
    // EAX交叉の準備
    eax::ObjectPools object_pools(tsp.city_count);
    eax::EAX_Rand eax_crossover(object_pools);
    
    // 世代数
    size_t num_generations = 2000;
    
    // 世代交代
    for (size_t generation = 0; generation < num_generations; ++generation) {
        // 集団をシャッフル
        shuffle(population.begin(), population.end(), rng);
        
        // ペアごとに交叉 (ERモデル)
        for (size_t i = 0; i + 1 < population_size; i += 2) {
            auto& parent1 = population[i];
            auto& parent2 = population[i + 1];

            // 子を生成（30個の子を試す）
            auto children = eax_crossover(parent1, parent2, 30, tsp, rng);

            // 親2個体 + 子個体を統一的に扱う
            vector<Individual::delta_t> candidates;
            candidates.reserve(children.size() + 2);
            candidates.emplace_back(parent1);
            candidates.emplace_back(parent2);
            for (auto& child : children) {
                candidates.emplace_back(parent1, std::move(child));
            }

            auto calc_distance = [](const Individual::delta_t& candidate) {
                return candidate.get_individual().get_distance() + candidate.get_delta().get_delta_distance();
            };

            // 評価値の良い順に2個体を選択
            vector<size_t> order(candidates.size());
            iota(order.begin(), order.end(), 0);
            partial_sort(order.begin(), order.begin() + 2, order.end(),
                [&](size_t a, size_t b) { return calc_distance(candidates[a]) < calc_distance(candidates[b]); });

            // 親個体と交換（バッファに適用）
            population[i] = candidates[order[0]];
            population[i + 1] = candidates[order[1]];

            // バッファを反映
            population[i].flush_buffer();
            population[i + 1].flush_buffer();
        }
        
        // 100世代ごとに進捗を表示
        if ((generation + 1) % 100 == 0) {
            auto best_it = min_element(population.begin(), population.end(),
                [](const auto& a, const auto& b) { return a.get_distance() < b.get_distance(); });
            cout << "Generation " << generation + 1 << ": Best distance = " << best_it->get_distance() << endl;
        }
    }
    
    // 最終結果の表示
    auto best_it = min_element(population.begin(), population.end(),
        [](const auto& a, const auto& b) { return a.get_distance() < b.get_distance(); });
    cout << "\nFinal best distance: " << best_it->get_distance() << endl;
    
    return 0;
}
