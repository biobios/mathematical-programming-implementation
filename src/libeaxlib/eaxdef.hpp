#pragma once

#include <cstdint>
#include <vector>
#include <array>

#include "tsp_loader.hpp"

namespace eax {
using adjacency_matrix_t = tsp::adjacency_matrix_t;
using NN_list_t = tsp::NN_list_t;

using ab_cycle_t = std::vector<size_t>;
using doubly_linked_list_t = std::vector<std::array<size_t, 2>>;
using edge_counts_t = std::vector<std::vector<size_t>>;

/**
 * @brief 双方向連結リストの読み取りが可能なコンセプト
 */
template <typename T>
concept doubly_linked_list_readable = requires(const T t) {
        { t[0][0] } -> std::convertible_to<size_t>;
        { t[0][1] } -> std::convertible_to<size_t>;
        { t.size() } -> std::convertible_to<size_t>;
    };

/**
 * @brief 双方向連結リストの書き込みが可能なコンセプト
 */
template <typename T>
concept doubly_linked_list_writable =
    doubly_linked_list_readable<T> && requires(T t, size_t val) {
        t[0][0] = val;
        t[0][1] = val;
    };

/**
 * @brief チェックサムの読み取りが可能なコンセプト
 */
template <typename T>
concept checksum_readable = requires(const T t) {
        { t.get_checksum() } -> std::convertible_to<uint64_t>;
    };

/**
 * @brief チェックサムの書き込みが可能なコンセプト
 */
template <typename T>
concept checksum_writable =
    checksum_readable<T> && requires(T t, uint64_t val) {
        t.set_checksum(val);
    };

/**
 * @brief 距離の読み取りが可能なコンセプト
 */
template <typename T>
concept distance_readable = requires(const T t) {
        { t.get_distance() } -> std::convertible_to<int64_t>;
    };

/**
 * @brief 距離の書き込みが可能なコンセプト
 */
template <typename T>
concept distance_writable =
    distance_readable<T> && requires(T t, int64_t val) {
        t.set_distance(val);
    };

/**
 * @brief 読み取り可能な個体コンセプト
 */
template <typename T>
concept individual_readable =
    doubly_linked_list_readable<T> &&
    checksum_readable<T> &&
    distance_readable<T>;

/**
 * @brief 書き込み可能な個体コンセプト
 */
template <typename T>
concept individual_writable =
    doubly_linked_list_writable<T> &&
    checksum_writable<T> &&
    distance_writable<T>;

/**
 * @brief すべてのABサイクルを見つけることが保証されたクラスのタグ
 */
struct complete_ABCycleFinder_tag {};

/**
 * @brief すべてのABサイクルを見つけることが保証されていないクラスのタグ
 */
struct incomplete_ABCycleFinder_tag {};

}