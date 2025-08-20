#include "crossover_delta.hpp"

namespace eax {
int64_t CrossoverDelta::get_delta_distance(const adjacency_matrix_t& adjacency_matrix) const {
    int64_t delta = 0;
    for (const auto& modification : modifications) {
        auto [v1, v2] = modification.edge1;
        size_t new_v2 = modification.new_v2;
        delta += adjacency_matrix[v1][new_v2] - adjacency_matrix[v1][v2];
    }
    
    return delta / 2; // 各エッジは2回カウントされるので、半分にする
}
}