#pragma once

#include <cstddef>
#include <cmath>

#include "eaxdef.hpp"
#include "crossover_delta.hpp"
#include "edge_counter.hpp"

namespace eax {

[[deprecated("This function is deprecated. Use the EdgeCounter version instead.")]]
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

template <typename Policy>
double calc_delta_entropy(const CrossoverDelta& child, EdgeCounter<Policy>& edge_counter, size_t pop_size) {
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
        uint64_t count_v1_v2 = edge_counter.get_edge_count(v1, v2);
        delta_H += calc_entropy(count_v1_v2 - 1, pop_size)
                    - calc_entropy(count_v1_v2, pop_size);
        edge_counter.decrement_edge_count(v1, v2);

        // v1 -> new_v2ができたなら
        uint64_t count_v1_new_v2 = edge_counter.get_edge_count(v1, new_v2);
        delta_H += calc_entropy(count_v1_new_v2 + 1, pop_size)
                    - calc_entropy(count_v1_new_v2, pop_size);
        edge_counter.increment_edge_count(v1, new_v2);
    }

    // もとに戻す
    for (auto it = child.get_modifications().rbegin(); it != child.get_modifications().rend(); ++it) {
        const auto& modification = *it;
        auto [v1, v2] = modification.edge1;
        size_t new_v2 = modification.new_v2;
        edge_counter.increment_edge_count(v1, v2);
        edge_counter.decrement_edge_count(v1, new_v2);
    }
    
    return delta_H;
}

namespace eval {
namespace delta {

namespace impl {
struct Entropy {
    [[deprecated("This function is deprecated. Use the EdgeCounter version instead.")]]
    double operator()(const CrossoverDelta& child, edge_counts_t& pop_edge_counts, size_t pop_size, double epsilon = 1e-9) const {
        double delta_L = child.get_delta_distance();
        if (delta_L > 0.0) {
            return -1.0;
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        double delta_H = calc_delta_entropy(child, pop_edge_counts, pop_size);
#pragma GCC diagnostic pop
        
        // 多様性が増すならば
        if (delta_H >= 0) {
            return -1.0 * delta_L / epsilon;
        }

        // 多様性が減るならば
        // 減少多様性当たりの距離の減少量を評価値とする
        return delta_L / delta_H;
    }

    template <typename Policy>
    double operator()(const CrossoverDelta& child, EdgeCounter<Policy>& edge_counter, size_t pop_size, double epsilon = 1e-9) const {
        double delta_L = child.get_delta_distance();
        if (delta_L > 0.0) {
            return -1.0;
        }

        double delta_H = calc_delta_entropy(child, edge_counter, pop_size);

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