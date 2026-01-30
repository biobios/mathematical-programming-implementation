#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

#include "tsp_loader.hpp"
#include "population_initializer.hpp"
#include "basic_individual.hpp"
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
    
    // BasicIndividualに変換
    vector<eax::BasicIndividual> population;
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
        
        // ペアごとに交叉
        for (size_t i = 0; i < population_size; ++i) {
            size_t parent1_idx = i;
            size_t parent2_idx = (i + 1) % population_size;
            
            auto& parent1 = population[parent1_idx];
            auto& parent2 = population[parent2_idx];
            
            // 子を生成（30個の子を試す）
            auto children = eax_crossover(parent1, parent2, 30, tsp, rng);
            
            if (!children.empty()) {
                // 最も改善した子を選択
                auto best_child_it = min_element(children.begin(), children.end(),
                    [](const auto& a, const auto& b) { return a.get_delta_distance() < b.get_delta_distance(); });
                
                // 改善していれば適用
                if (best_child_it->get_delta_distance() < 0) {
                    best_child_it->apply_to(parent1);
                }
            }
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
