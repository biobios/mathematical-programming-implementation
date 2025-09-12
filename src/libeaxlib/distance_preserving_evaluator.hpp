#pragma once

#include "eaxdef.hpp"
#include "crossover_delta.hpp"

namespace eax {
namespace eval {
namespace delta {
    struct DistancePreserving {
        double operator()(const CrossoverDelta& child, const adjacency_matrix_t& adjacency_matrix, edge_counts_t& pop_edge_counts, double epsilon = 1e-9) const {
            double delta_L = child.get_delta_distance(adjacency_matrix);
            
            if (delta_L >= 0.0) {
                return -1.0;
            }

            double delta_H = 0;
            
            for (const auto& modification : child.get_modifications()) {
                auto [v1, v2] = modification.edge1;
                size_t new_v2 = modification.new_v2;
                
                delta_H -= pop_edge_counts[v1][v2] - 1;
                --pop_edge_counts[v1][v2];

                delta_H += pop_edge_counts[v1][new_v2];
                ++pop_edge_counts[v1][new_v2];
            }
            
            for (const auto& modification : child.get_modifications()) {
                auto [v1, v2] = modification.edge1;
                size_t new_v2 = modification.new_v2;
                
                ++pop_edge_counts[v1][v2];
                --pop_edge_counts[v1][new_v2];
            }

            // 多様性が増すならば
            if (delta_H >= 0) {
                return -1.0 * delta_L / epsilon;
            }

            // 多様性が減るならば
            // 減少多様性当たりの距離の減少量を評価値とする
            return delta_L / delta_H;
        }
    };
}
} // namespace eval
} // namespace eax
