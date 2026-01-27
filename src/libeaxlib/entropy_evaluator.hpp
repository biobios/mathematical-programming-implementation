#pragma once

#include <cstddef>
#include <cmath>

#include "eaxdef.hpp"
#include "crossover_delta.hpp"

namespace eax {
inline double calc_delta_entropy(const CrossoverDelta& child, edge_counts_t& pop_edge_counts, size_t pop_size) {
    double delta_H = 0.0;

    auto calc_entropy = [](size_t count, size_t pop_size) {
        if (count == 0) {
            return 0.0;
        }
        double ratio = static_cast<double>(count) / pop_size;
        return -ratio * std::log2(ratio);
    };

    for (const auto& modification : child.get_modifications()) {
        auto [v1, v2] = modification.edge1;
        size_t new_v2 = modification.new_v2;

        // v1 -> v2が消えたなら
        delta_H += calc_entropy(pop_edge_counts[v1][v2] - 1, pop_size)
                    - calc_entropy(pop_edge_counts[v1][v2], pop_size);

        pop_edge_counts[v1][v2] -= 1;

        // v1 -> new_v2ができたなら
        delta_H += calc_entropy(pop_edge_counts[v1][new_v2] + 1, pop_size)
                    - calc_entropy(pop_edge_counts[v1][new_v2], pop_size);

        pop_edge_counts[v1][new_v2] += 1;
    }

    // もとに戻す
    for (const auto& modification : child.get_modifications()) {
        auto [v1, v2] = modification.edge1;
        size_t new_v2 = modification.new_v2;
        pop_edge_counts[v1][v2] += 1;
        pop_edge_counts[v1][new_v2] -= 1;
    }
    
    return delta_H;
}

namespace eval {
namespace delta {

namespace impl {
struct Entropy {
    double operator()(const CrossoverDelta& child, edge_counts_t& pop_edge_counts, size_t pop_size, double epsilon = 1e-9) const {
        double delta_L = child.get_delta_distance();
        if (delta_L > 0.0) {
            return -1.0;
        }

        double delta_H = calc_delta_entropy(child, pop_edge_counts, pop_size);
        
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

constexpr impl::Entropy Entropy{};
}
}
}