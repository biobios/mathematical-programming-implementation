#pragma once

#include <cstddef>
#include <utility>
#include <stdexcept>

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

    CrossoverDelta(const individual_readable auto& individual)
        :   modifications(),
            base_checksum(individual.get_checksum()),
            delta_checksum(0),
            delta_distance(0) {}
    
    /**
     * @param modifications 変更履歴
     * @param base_checksum ベースの個体のチェックサム
     * @param delta_distance 距離の変化
     * @pre modifications.size() % 2 == 0
     */
    CrossoverDelta(std::vector<Modification>&& modifications, uint64_t base_checksum, int64_t delta_distance)
        :   modifications(std::move(modifications)),
            base_checksum(base_checksum),
            delta_checksum(compute_delta_checksum(this->modifications)),
            delta_distance(delta_distance) {}
    
    /**
     * @brief 変更を個体に適用する
     */
    void apply_to(individual_writable auto& individual) const {

        if (base_checksum != individual.get_checksum()) {
            throw std::invalid_argument("CrossoverDelta::apply_to: The base checksum does not match the individual's checksum.");
        }

        for (const auto& modification : modifications) {
            auto [v1, v2] = modification.edge1;
            size_t new_v2 = modification.new_v2;
            if (individual[v1][0] == v2) {
                individual[v1][0] = new_v2;
            } else {
                individual[v1][1] = new_v2;
            }
        }
        
        uint64_t new_checksum = base_checksum ^ delta_checksum;
        individual.set_checksum(new_checksum);

        int64_t new_distance = individual.get_distance() + delta_distance;
        individual.set_distance(new_distance);
    }

    /**
     * @brief 変更を元に戻す
     */
    void undo(individual_writable auto& individual) const {

        if ((base_checksum ^ delta_checksum) != individual.get_checksum()) {
            throw std::invalid_argument("CrossoverDelta::undo: The individual's checksum does not match the expected checksum after applying the delta.");
        }

        for (auto it = modifications.rbegin(); it != modifications.rend(); ++it) {
            const auto& modification = *it;
            auto [v1, v2] = modification.edge1;
            size_t new_v2 = modification.new_v2;
            if (individual[v1][0] == new_v2) {
                individual[v1][0] = v2;
            } else {
                individual[v1][1] = v2;
            }
        }
        
        uint64_t original_checksum = base_checksum;
        individual.set_checksum(original_checksum);

        int64_t original_distance = individual.get_distance() - delta_distance;
        individual.set_distance(original_distance);
    }

    /**
     * @brief ベースの個体であるかどうかをチェックする(apply_toを呼び出せるかどうか)
     * @param individual チェックする個体
     * @return ベースの個体であればtrue、そうでなければfalse
     */
    bool is_base_individual(const individual_readable auto& individual) const {
        return base_checksum == individual.get_checksum();
    }

    /**
     * @brief 変更後の個体であるかどうかをチェックする(undoを呼び出せるかどうか)
     * @param individual チェックする個体
     * @return 変更後の個体であればtrue、そうでなければfalse
     */
    bool is_modified_individual(const individual_readable auto& individual) const {
        return (base_checksum ^ delta_checksum) == individual.get_checksum();
    }
    
    /**
     * @brief 変更による距離の変化を取得する (計算量: O(m), m = modifications.size())
     * @param adjacency_matrix 隣接行列
     * @return 距離の変化
     */
    int64_t get_delta_distance() const;

    /**
     * @brief 変更履歴を取得する
     * @return 変更履歴の参照
     */
    const std::vector<Modification>& get_modifications() const {
        return modifications;
    }

private:
    /**
     * @brief 変更履歴
     */
    std::vector<Modification> modifications;
    
    /**
     * @brief ベースの個体のチェックサム
     */
    uint64_t base_checksum;

    /**
     * @brief チェックサムの更新値
     */
    uint64_t delta_checksum;

    /**
     * @brief 距離の変化
     */
    int64_t delta_distance;
    
    /**
     * @brief チェックサムの更新値を計算する
     */
    static uint64_t compute_delta_checksum(const std::vector<Modification>& modifications);
};
}