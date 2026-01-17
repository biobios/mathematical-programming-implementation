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
 * @brief チェックサムコンセプト
 */
template <typename T>
concept has_checksum = requires(T t) {
    { t.checksum() } -> std::convertible_to<uint64_t>;
    t.checksum() = 0; // 変更可能であることを確認
};

/**
 * @brief 距離コンセプト
 */
template <typename T>
concept has_distance = requires(T t) {
    { t.distance() } -> std::convertible_to<int64_t>;
    t.distance() = 0; // 変更可能であることを確認
};

/**
 * @brief 個体コンセプト
 */
template <typename T>
concept individual_concept = doubly_linked_list_like<T> && has_checksum<T> && has_distance<T>;

/**
 * @brief すべてのABサイクルを見つけることが保証されたクラスのタグ
 */
struct complete_ABCycleFinder_tag {};

/**
 * @brief すべてのABサイクルを見つけることが保証されていないクラスのタグ
 */
struct incomplete_ABCycleFinder_tag {};

}