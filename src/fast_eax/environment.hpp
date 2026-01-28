#pragma once

#include <vector>
#include <functional>
#include <chrono>

#include "tsp_loader.hpp"
#include "object_pool.hpp"
#include "limited_range_integer_set.hpp"
#include "individual_with_pending_delta.hpp"

namespace eax {
    using Individual = IndividualWithPendingDelta;

    enum class EAXType {
        Rand,
        N_AB,
    };

    enum class SelectionType {
        Greedy,
        LDL,
        Ent,
    };

    struct Environment {
        tsp::TSP tsp;
        size_t N_parameter;
        size_t population_size;
        EAXType eax_type;
        SelectionType selection_type;
        std::vector<std::vector<size_t>> pop_edge_counts; // 各エッジの個数
        std::mt19937 random_gen;

        void set_initial_edge_counts(const std::vector<Individual>& init_pop) {
            pop_edge_counts.resize(tsp.city_count, std::vector<size_t>(tsp.city_count, 0));
            
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