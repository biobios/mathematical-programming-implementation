#pragma once

#include <vector>

#include "tsp_loader.hpp"

namespace eax {
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
        
        void update_edge_counts(const std::vector<Individual>& population) {
            pop_edge_counts.assign(tsp.city_count, std::vector<size_t>(tsp.city_count, 0));
            for (const auto& individual : population) {
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