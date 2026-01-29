#pragma once

#include <vector>
#include <algorithm>
#include <ranges>

#include "eaxdef.hpp"
#include "crossover_delta.hpp"

namespace eax {
/**
 * @brief 辺の出現回数を出現回数で降順ソートして管理するクラス
 * @details
 *     各頂点について、接続先頂点ごとの辺の出現回数を管理する。
 *     メモリ使用量は O(n * m) で、n は頂点数、m は個体数。
 */
class EdgeCounter {
public:
    EdgeCounter(size_t num_vertices, size_t population_size)
        : vertex_counters(num_vertices, VertexEdgeCounter{population_size}) {}
    
    template <doubly_linked_list_readable Individual>
    EdgeCounter(const std::vector<Individual>& population)
        : EdgeCounter(population[0].size(), population.size()) {
        
        for (const auto& individual : population) {
            for (size_t v1 = 0; v1 < individual.size(); ++v1) {
                increment_edge_count(v1, individual[v1][0]);
                increment_edge_count(v1, individual[v1][1]);
            }
        }
    }

    /**
     * @brief CrossoverDeltaで表される変更を適用して辺の出現回数を更新する
     * @param delta 適用する変更
     */
    void apply_crossover_delta(const CrossoverDelta& delta) {
        for (const auto& modification : delta.get_modifications()) {
            auto [v1, v2] = modification.edge1;
            size_t new_v2 = modification.new_v2;
            decrement_edge_count(v1, v2);
            increment_edge_count(v1, new_v2);
        }
    }

    /**
     * @brief 頂点v1から頂点v2への辺の出現回数を取得する
     * @param v1 始点頂点
     * @param v2 終点頂点
     * @return 出現回数
     */
    size_t get_edge_count(size_t v1, size_t v2) const {
        return vertex_counters[v1].get_edge_count(v2);
    }

    /**
     * @brief 頂点v1から接続されている頂点の接続数で降順ソートされたvectorを取得する
     * @param v1 始点頂点
     * @return 接続されている頂点のvector
     */
    const std::vector<size_t>& get_connected_vertices(size_t v1) const {
        return vertex_counters[v1].connected_vertices;
    }
    
    /**
     * @brief 重複を除いた辺の数を取得する
     * @details
     *  (a, b) と (b, a) は同一の辺として数える
     * @return 重複を除いた辺の数
     */
    size_t get_unique_edge_count() const {
        return unique_edge_count / 2;
    }

private:
    /**
     * @brief 頂点１つについて管理するクラス
     */
    class VertexEdgeCounter {
    public:
        VertexEdgeCounter(size_t population_size)
            : connected_vertices(), count_range_begins() {
            connected_vertices.reserve(population_size * 2);
            count_range_begins.resize(population_size, 0);
        }

        /**
         * @brief 頂点v2への辺の出現回数をインクリメントする
         * @param v2 接続先頂点
         * @details
         *      最悪計算量: O(m), m = population_size
         */
        void increment_edge_count(size_t v2) {
            auto it = std::find(connected_vertices.begin(), connected_vertices.end(), v2);

            if (it == connected_vertices.end()) {
                connected_vertices.push_back(v2);
                return;
            }

            size_t v2_index = std::distance(connected_vertices.begin(), it);

            // itが属する出現回数の範囲の開始位置を探索
            // v2_index 以下の要素のうち最初のものを探す
            // (count_iterators.last() は 0 であるため、必ず見つかる)
            auto range_begin_itr = std::lower_bound(
                count_range_begins.begin(), count_range_begins.end(), v2_index,
                [](const auto& lhs, const auto& rhs) {
                    return lhs > rhs;
                });

            auto& range_begin = *range_begin_itr;

            // 頂点番号を移動して、範囲を更新(縮小)
            if (range_begin != v2_index) {
                std::swap(connected_vertices[range_begin], connected_vertices[v2_index]);
            }
            ++range_begin;
        }
        
        /**
         * @brief 頂点v2への辺の出現回数をデクリメントする
         * @param v2 接続先頂点
         * @details
         *      最悪計算量: O(m), m = population_size
         */
        void decrement_edge_count(size_t v2) {
            auto it = std::find(connected_vertices.begin(), connected_vertices.end(), v2);
            if (it == connected_vertices.end()) {
                return;
            }

            size_t v2_index = std::distance(connected_vertices.begin(), it);

            // itが属する出現回数の範囲の開始位置を探索
            auto range_begin_itr = std::lower_bound(
                count_range_begins.begin(), count_range_begins.end(), v2_index,
                [](const auto& lhs, const auto& rhs) {
                    return lhs > rhs;
                });
            
            if (range_begin_itr == count_range_begins.begin()) {
                // 出現回数1回の範囲にある場合、頂点を削除
                std::swap(*it, connected_vertices.back());
                connected_vertices.pop_back();
                return;
            }

            auto& prev_range_begin = *(range_begin_itr - 1);
            // 範囲を更新(拡大)して頂点番号を移動
            --prev_range_begin;
            if (prev_range_begin != v2_index) {
                std::swap(connected_vertices[prev_range_begin], connected_vertices[v2_index]);
            }
        }

        /**
         * @brief 頂点v2への辺の出現回数を取得する
         * @param v2 接続先頂点
         * @return 出現回数
         * @details
         *      計算量: O(m), m = population_size
         */
        size_t get_edge_count(size_t v2) const {
            auto it = std::find(connected_vertices.begin(), connected_vertices.end(), v2);
            if (it == connected_vertices.end()) {
                return 0;
            }

            size_t v2_index = std::distance(connected_vertices.begin(), it);

            // itが属する出現回数の範囲の開始位置を探索
            auto range_begin_itr = std::lower_bound(
                count_range_begins.begin(), count_range_begins.end(), v2_index,
                [](const auto& lhs, const auto& rhs) {
                    return lhs > rhs;
                });

            return std::distance(count_range_begins.begin(), range_begin_itr) + 1;
        }

    private:
        /**
         * @brief 1回以上出現する辺の接続先
         */
        std::vector<size_t> connected_vertices;
        
        /**
         * @brief connected_verticesに対応する出現回数の範囲の開始位置
         * @details [count_range_begins[i], count_range_begins[i - 1]) の範囲にある頂点は
         *          出現回数が i + 1 回であることを示す
         *          もし、[count_range_begins[0], connected_vertices.size()) の範囲に
         *          頂点が存在する場合、その頂点は出現回数が 1 回であることを示す。
         */
        std::vector<size_t> count_range_begins;

        friend class EdgeCounter;
    };

    /**
     * @brief 頂点v1から頂点v2への辺の出現回数をインクリメントする
     * @param v1 始点頂点
     * @param v2 終点頂点
     */
    void increment_edge_count(size_t v1, size_t v2) {

        std::size_t prev_size = vertex_counters[v1].connected_vertices.size();

        vertex_counters[v1].increment_edge_count(v2);

        std::size_t new_size = vertex_counters[v1].connected_vertices.size();
        unique_edge_count += new_size - prev_size;
    }

    /**
     * @brief 頂点v1から頂点v2への辺の出現回数をデクリメントする
     * @param v1 始点頂点
     * @param v2 終点頂点
     */
    void decrement_edge_count(size_t v1, size_t v2) {

        std::size_t prev_size = vertex_counters[v1].connected_vertices.size();

        vertex_counters[v1].decrement_edge_count(v2);
        
        std::size_t new_size = vertex_counters[v1].connected_vertices.size();
        unique_edge_count -= prev_size - new_size;
    }

    /**
     * @brief 各頂点の辺の出現回数を管理する配列
     */
    std::vector<VertexEdgeCounter> vertex_counters;

    /**
     * @brief 重複を除いた有向辺の数を記録する
     * @details 
     *  頂点 v1 ごとに接続先 v2 の集合のサイズの総和を表す内部カウンタ。
     *  具体的には、(a, b) と (a, b) は同じ辺として数えるが、
     *  (a, b) と (b, a) は別の辺として数える（有向辺として扱う）。
     *  無向辺数が必要な場合は、get_unique_edge_count() の戻り値を利用すること。
     */
    std::size_t unique_edge_count = 0;
};
}