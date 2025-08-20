#pragma once

#include "crossover_delta.hpp"

namespace eax {
namespace eval {
namespace delta {
    struct Greedy {
        double operator()(const CrossoverDelta& child, const adjacency_matrix_t& adjacency_matrix) const {
            return -1.0 * child.get_delta_distance(adjacency_matrix);
        }
    };
}
}
}