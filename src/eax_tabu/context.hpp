#pragma once

#include <vector>
#include <functional>
#include <chrono>
#include <random>

#include "edge_counter.hpp"
#include "tsp_loader.hpp"
#include "object_pool.hpp"
#include "limited_range_integer_set.hpp"
#include "tabu_individual.hpp"
#include "eax_tabu.hpp"
#include "eax_rand.hpp"
#include "eax_n_ab.hpp"
#include "eax_uniform.hpp"

namespace eax {
    using Individual = TabuIndividual;

    enum class SelectionType {
        Greedy,
        Ent,
        DistancePreserving,
    };
    
    using eax_type_t = std::variant<EAX_Rand_tag, EAX_n_AB_tag, EAX_full_UNIFORM_tag>;

    
    struct Environment {
        tsp::TSP tsp;
        size_t population_size;
        size_t num_children;
        SelectionType selection_type;
        std::mt19937::result_type random_seed;
        eax_type_t eax_type;
    };

    struct Context {
        Environment env;

        EdgeCounter<> pop_edge_counts; // 各エッジの個数
        std::mt19937 random_gen;

        // 最良解の長さ
        size_t best_length = 1e18;
        // 最良解に到達した世代
        size_t generation_of_reached_best = 0;
        // 停滞した世代数
        size_t stagnation_generations = 0;
        // 現在の世代数
        size_t current_generation = 0;
        // 最終世代
        size_t final_generation = 0;
        
        // 計測開始時刻 (これはシリアライズされない)
        std::chrono::system_clock::time_point start_time;
        // 経過時間
        double elapsed_time = 0.0;
        // エントロピー
        double entropy;

        Context(const Environment& environment, const std::vector<Individual>& initial_population) 
            : env(environment), pop_edge_counts(initial_population), random_gen(environment.random_seed), entropy(pop_edge_counts.calc_entropy())
        {}
        
    };
}