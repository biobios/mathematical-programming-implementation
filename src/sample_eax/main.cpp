#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <cmath>

#include "tsp_loader.hpp"
#include "population_initializer.hpp"
#include "eax_tabu.hpp"
#include "parent_reference_merger.hpp"
#include "entropy_evaluator.hpp"
#include "object_pools.hpp"
#include "sample_tabu_individual.hpp"

int main() {
    using namespace std;
    
    // TSPファイルの読み込み
    tsp::TSP tsp = tsp::TSP_Loader::load_tsp("att532.tsp");
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
    
    using Individual = eax::SampleTabuIndividual;

    // SampleTabuIndividualに変換
    vector<Individual> population;
    population.reserve(initial_paths.size());
    for (const auto& path : initial_paths) {
        population.emplace_back(path, tsp.adjacency_matrix);
    }
    
    cout << "Initial population created." << endl;
    cout << "Initial best distance: " << min_element(population.begin(), population.end(),
        [](const auto& a, const auto& b) { return a.get_distance() < b.get_distance(); })->get_distance() << endl;
    
    // エッジ頻度の初期化
    vector<vector<size_t>> pop_edge_counts(tsp.city_count, vector<size_t>(tsp.city_count, 0));
    for (const auto& individual : population) {
        for (size_t i = 0; i < individual.size(); ++i) {
            size_t v1 = individual[i][0];
            size_t v2 = individual[i][1];
            pop_edge_counts[i][v1] += 1;
            pop_edge_counts[i][v2] += 1;
        }
    }
    
    // EAX交叉の準備
    eax::ObjectPools object_pools(tsp.city_count);
    
    // ParentReferenceMerger を使うカスタムEAX型
    using EAX_tabu_N_AB_with_ParentRef = eax::EAX_tabu<eax::N_AB_e_set_assembler_builder, eax::ParentReferenceMerger>;
    EAX_tabu_N_AB_with_ParentRef eax_tabu_n_ab(object_pools);
    
    // 参照親の範囲
    
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
            const auto& parent2 = population[parent2_idx];
            
            // 参照親を選択（population から20個をランダムに選択）
            std::vector<std::reference_wrapper<const Individual>> reference_parents;
            reference_parents.reserve(20);
            for (size_t j = 0; j < 20; ++j) {
                size_t idx = (parent1_idx + 2 + j) % population_size;
                reference_parents.emplace_back(population[idx]);
            }
            
            // タブーエッジを取得
            const auto& tabu_edges = parent1.get_tabu_edges();
            
            // EAX_tabu_N_ABで子を生成
            auto children = eax_tabu_n_ab(parent1, parent2, 30, tsp, rng,
                std::make_tuple(5),  // N=5個のABサイクル（BuilderArgsTuple）
                std::forward_as_tuple(reference_parents),  // MergerArgsTuple に参照親を参照渡し
                std::forward_as_tuple(tabu_edges)  // FinderArgsTuple にタブーエッジを参照渡し
            );
            
            if (!children.empty()) {
                // エントロピー評価を一度だけ計算して最良の子を選択
                const eax::CrossoverDelta* best_child = nullptr;
                double best_eval = -1.0;

                for (const auto& child : children) {
                    double eval = eax::eval::delta::Entropy(child, pop_edge_counts, population_size);
                    if (eval > best_eval) {
                        best_eval = eval;
                        best_child = &child;
                    }
                }

                // 評価値が正（改善している）なら適用
                if (best_child != nullptr && best_eval > 0.0) {
                    best_child->apply_to(parent1);

                    for (const auto& modification : best_child->get_modifications()) {
                        auto [v1, v2] = modification.edge1;
                        size_t new_v2 = modification.new_v2;
                        pop_edge_counts[v1][v2] -= 1;
                        pop_edge_counts[v1][new_v2] += 1;
                    }
                    
                    // タブーリストを更新
                    parent1.update_tabu(*best_child, rng);
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
