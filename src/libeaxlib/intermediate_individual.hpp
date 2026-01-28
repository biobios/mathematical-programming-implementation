#pragma once

#include <limits>

#include "eaxdef.hpp"
#include "crossover_delta.hpp"

namespace eax {

/**
 * @invariant modifications.size() % 2 == 0
 */
class IntermediateIndividual {
public:
    /**
     * @brief 指定したサイズの中間個体を構築する
     * @param size 個体のサイズ
     * @details 頂点iの隣接頂点は(i+1) % sizeと(i+size-1) % sizeに初期化される
     */
    IntermediateIndividual(size_t size);

    /**
     * @brief 指定した個体をもとに中間個体を構築する
     * @param individual 元にする個体
     */
    IntermediateIndividual(const individual_readable auto& individual)
        : individual_being_edited(individual.size()),
        base_checksum(individual.get_checksum()),
        modifications(),
        path(individual.size()),
        pos(individual.size()) {
        assign(individual);
    }

    /**
     * @brief 指定した個体を中間個体に代入する
     * @param individual 代入する個体
     */
    void assign(const individual_readable auto& individual) {
        reset();
        base_checksum = individual.get_checksum();
        for (size_t i = 0; i < individual.size(); ++i) {
            individual_being_edited[i] = {individual[i][0], individual[i][1]};
        }

        size_t prev = 0;
        size_t current = 0;
        for (size_t i = 0; i < individual.size(); ++i) {
            path[i] = current;
            pos[current] = i;

            size_t next = individual[current][0];
            if (next == prev) {
                next = individual[current][1];
            }

            prev = current;
            current = next;
        }
    }
    
    /**
     * @brief 現在の変更内容を取得し、中間個体を元に戻す
     * @param adjacency_matrix 隣接行列
     * @return 変更内容
     */
    CrossoverDelta get_delta_and_revert(const adjacency_matrix_t& adjacency_matrix);
    /**
     * @brief 現在の変更内容を破棄し、中間個体を元に戻す
     */
    void discard();
    
    /**
     * @brief 指定したインデックスの頂点の隣接頂点を取得する
     * @param index 頂点のインデックス
     * @return 頂点の隣接頂点の配列
     */
    const std::array<size_t, 2>& operator[](size_t index) const;
    
    /**
     * @brief 指定した2つの辺を入れ替える
     * @param edge1 入れ替える辺1
     * @param edge2 入れ替える辺2
     * @details
     * edge1 = (v1, v2), edge2 = (u1, u2) のとき、以下のように変更する。
     * - v1 <-> u1
     * - v2 <-> u2
     */
    void swap_edges(std::pair<size_t, size_t> edge1, std::pair<size_t, size_t> edge2);

    /**
     * @brief 指定したABサイクル群を中間個体に適用する
     * @tparam ABCycles ABサイクル群の型
     * @param AB_cycles 適用するABサイクル群
     */
    template <std::ranges::range ABCycles>
        requires std::convertible_to<std::ranges::range_value_t<ABCycles>, const ab_cycle_t&>
    void apply_AB_cycles(const ABCycles& AB_cycles) {
        using namespace std;

        auto edge_swap = [this](size_t b1, size_t ba, size_t ab, size_t b2) {
            change_connection(ba, ab, b1);
            change_connection(ab, ba, b2);
        };

        for (const ab_cycle_t& cycle : AB_cycles) {
            for (size_t i = 2; i < cycle.size() - 2; i += 2) {
                edge_swap(cycle[i - 1], cycle[i], cycle[i + 1], cycle[i + 2]);
            }
            // i = 0
            {
                edge_swap(cycle[cycle.size() - 1], cycle[0], cycle[1], cycle[2]);
            }
            // i = cycle.size() - 2
            {
                edge_swap(cycle[cycle.size() - 3], cycle[cycle.size() - 2], cycle[cycle.size() - 1], cycle[0]);
            }
        }
    }

    /**
     * @brief 現在の個体の巡回路の順序を取得する
     * @return 巡回路の順序を表す頂点のベクター
     */
    const std::vector<size_t>& get_path() const;
    /**
     * @brief 現在の個体の各頂点の巡回路における位置を取得する
     * @return 各頂点の巡回路における位置を表すベクター
     */
    const std::vector<size_t>& get_pos() const;
    /**
     * @brief 中間個体の元の個体に対する距離の変化を計算する
     * @param adjacency_matrix 隣接行列
     * @return 距離の変化
     */
    int64_t calc_delta_distance(const std::vector<std::vector<int64_t>>& adjacency_matrix) const;
    size_t size() const;
private:
    /**
     * @brief v1とv2の接続をv1とnew_v2の接続に変更する
     * @param v1 変更する辺の一方の頂点
     * @param v2 変更する辺のもう一方の頂点
     * @param new_v2 v1と接続する新しい頂点
     * @note modificationsの条件を満たすように呼び出すこと
     */
    void change_connection(size_t v1, size_t v2, size_t new_v2);

    void revert();
    void reset();
    void undo(const CrossoverDelta::Modification& modification);
    doubly_linked_list_t individual_being_edited;
    uint64_t base_checksum;
    /**
     * 個体の変更履歴
     * 偶数番目あるいは奇数番目の要素だけを見れば、交叉に関わったすべての辺を知ることができる
     */
    std::vector<CrossoverDelta::Modification> modifications;
    std::vector<size_t> path;
    std::vector<size_t> pos;
};
}