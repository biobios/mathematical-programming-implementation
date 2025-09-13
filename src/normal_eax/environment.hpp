#pragma once

#include <vector>
#include <functional>
#include <chrono>

#include "tsp_loader.hpp"
#include "object_pool.hpp"
#include "limited_range_integer_set.hpp"
#include "individual.hpp"

namespace eax {
    enum class EAXType {
        Rand,
        N_AB,
        Block2,
    };

    enum class SelectionType {
        Greedy,
        Ent,
        DistancePreserving,
    };

    struct Environment {
        tsp::TSP tsp;
        size_t N_parameter;
        size_t population_size;
        size_t num_children;
        EAXType eax_type;
        SelectionType selection_type;
        std::vector<std::vector<size_t>> pop_edge_counts; // 各エッジの個数

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