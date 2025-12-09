#pragma once

#include <cstddef>
#include <utility>

#include "eaxdef.hpp"

namespace eax {
/**
 * @brief 交叉操作の変更履歴を表すクラス
 */
class CrossoverDelta {
public:
    /**
     * @brief 交叉操作の変更内容
     */
    struct Modification {
        /**
         * @brief 変更前の辺 (v1, v2)
         */
        std::pair<size_t, size_t> edge1;
        /**
         * @brief 変更後にv1に接続される新しい頂点
         */
        size_t new_v2;
    };
    CrossoverDelta() = default;
    
    /**
     * @param modifications 変更履歴
     * @pre modifications.size() % 2 == 0
     */
    CrossoverDelta(std::vector<Modification>&& modifications)
        : modifications(std::move(modifications)) {}
    
    /**
     * @brief 変更を個体に適用する
     * @tparam T 個体の型
     */
    template <doubly_linked_list_like T>
    void apply_to(T& individual) const {
        for (const auto& modification : modifications) {
            auto [v1, v2] = modification.edge1;
            size_t new_v2 = modification.new_v2;
            if (individual[v1][0] == v2) {
                individual[v1][0] = new_v2;
            } else {
                individual[v1][1] = new_v2;
            }
        }
    }
    
    /**
     * @brief 変更による距離の変化を取得する (計算量: O(m), m = modifications.size())
     * @param adjacency_matrix 隣接行列
     * @return 距離の変化
     */
    int64_t get_delta_distance(const adjacency_matrix_t& adjacency_matrix) const;

    /**
     * @brief 変更履歴を取得する
     * @return 変更履歴の参照
     */
    const std::vector<Modification>& get_modifications() const {
        return modifications;
    }

private:
    std::vector<Modification> modifications;
};
}