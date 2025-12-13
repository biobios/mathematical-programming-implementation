#pragma once

#include <cstdint>
#include <vector>
#include <array>

namespace eax {
using adjacency_matrix_t = std::vector<std::vector<int64_t>>;
using NN_list_t = std::vector<std::vector<size_t>>;
using ab_cycle_t = std::vector<size_t>;
using doubly_linked_list_t = std::vector<std::array<size_t, 2>>;
using edge_counts_t = std::vector<std::vector<size_t>>;

template <typename T>
concept doubly_linked_list_like = requires(T t) {
    { t[0][0] } -> std::convertible_to<size_t>;
    { t[0][1] } -> std::convertible_to<size_t>;
    { t.size() } -> std::convertible_to<size_t>;
};

/**
 * @brief すべてのABサイクルを見つけることが保証されたクラスのタグ
 */
struct complete_ABCycleFinder_tag {};

/**
 * @brief すべてのABサイクルを見つけることが保証されていないクラスのタグ
 */
struct incomplete_ABCycleFinder_tag {};

}