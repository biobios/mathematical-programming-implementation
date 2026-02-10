#pragma once

#include <vector>
#include <functional>
#include <chrono>
#include <random>

#include "tsp_loader.hpp"
#include "object_pool.hpp"
#include "limited_range_integer_set.hpp"
#include "individual_with_pending_delta.hpp"
#include "edge_counter.hpp"

namespace eax {
    using Individual = IndividualWithPendingDelta;

    enum class EAXType {
        One_AB,
        Block2,
    };

    enum class SelectionType {
        Greedy,
        Ent,
        DistancePreserving,
    };
    
    struct Environment {
        tsp::TSP tsp;
        size_t population_size;
        size_t num_children;
        SelectionType selection_type;
        std::mt19937::result_type random_seed;
        size_t merge_range;
    };

    struct Context {
        Environment env;
        
        EdgeCounter<> edge_counter;

        EAXType eax_type = EAXType::One_AB;
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
        // 現在の世代数
        size_t current_generation = 0;
        // 最終世代
        size_t final_generation = 0;

        // GAの段階
        enum class GA_Stage {
            Stage1,
            Stage2,
        };
        GA_Stage stage = GA_Stage::Stage1;
        
        // 計測開始時刻 (これはシリアライズされない)
        std::chrono::system_clock::time_point start_time{};
        // 経過時間
        double elapsed_time = 0.0;
        // エントロピー
        double entropy;

        Context(const Environment& environment, const std::vector<Individual>& initial_population)
            : env(environment),
              edge_counter(initial_population),
              random_gen(environment.random_seed),
              entropy(edge_counter.calc_entropy()) {}
    };
}