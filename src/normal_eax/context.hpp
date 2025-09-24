#pragma once

#include <vector>
#include <functional>
#include <chrono>
#include <random>

#include "tsp_loader.hpp"
#include "object_pool.hpp"
#include "limited_range_integer_set.hpp"
#include "individual.hpp"

namespace eax {
    enum class EAXType {
        N_AB,
        Block2,
    };

    enum class SelectionType {
        Greedy,
        Ent,
        DistancePreserving,
    };

    struct Context {
        struct {
            tsp::TSP tsp;
            size_t population_size;
            size_t num_children;
            SelectionType selection_type;
            // 交叉後にエッジカウントを更新する必要があるかどうか
            bool need_to_update_edge_counts;
        } env;

        EAXType eax_type;
        std::vector<std::vector<size_t>> pop_edge_counts; // 各エッジの個数
        std::mt19937 random_gen;

        // 最良解の長さ
        size_t best_length = 1e18;
        // 最良解に到達した世代
        size_t generation_of_reached_best = 0;
        // 停滞した世代数
        size_t stagnation_generations = 0;
        // Block2(Stage2)に移行した世代
        size_t generation_of_transition_to_stage2 = 0;
        // ステージ遷移に用いる変数
        size_t G_devided_by_10 = 0;
        // 最終世代
        size_t final_generation = 0;

        // GAの段階
        enum class GA_Stage {
            Stage1,
            Stage2,
        };
        GA_Stage stage = GA_Stage::Stage1;


        void set_initial_edge_counts(const std::vector<Individual>& init_pop) {
            pop_edge_counts.resize(env.tsp.city_count, std::vector<size_t>(env.tsp.city_count, 0));
            
            for (const auto& individual : init_pop) {
                for (size_t i = 0; i < individual.size(); ++i) {
                    size_t v1 = individual[i][0];
                    size_t v2 = individual[i][1];
                    pop_edge_counts[i][v1] += 1;
                    pop_edge_counts[i][v2] += 1;
                }
            }
        };
    };
}