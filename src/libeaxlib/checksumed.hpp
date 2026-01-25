#pragma once

#include <cstdint>

namespace eax {
/**
 * @brief 容易にチェックサム付き個体を定義できるようにするためのベースクラス
 * @details
 *  使用法:
 * ```cpp
 * class MyIndividual : public Checksumed {
 *     // 個体の実装
 * }
 * ```
 */
class Checksumed {
public:
    Checksumed()
        : checksum_value(calc_checksum_from_seed(global_checksum_counter++)) {}

    /**
     * @brief 個体のチェックサムを取得する
     * @return チェックサム
     */
    uint64_t get_checksum() const {
        return checksum_value;
    }

    /**
     * @brief 個体のチェックサムを設定する
     * @param val 設定するチェックサムの値
     */
    void set_checksum(uint64_t val) {
        checksum_value = val;
    }
private:
    /**
     * @brief チェックサムの重複を避けるためのカウンタ
     */
    static inline uint64_t global_checksum_counter = 1;

    /**
     * @brief シード値からチェックサムを生成する
     * @param seed シード値
     * @return チェックサム
     */
    static uint64_t calc_checksum_from_seed(uint64_t seed) {
        // 単純な線形合同法を使用してチェックサムを生成
        constexpr uint64_t A = 6364136223846793005ULL;
        constexpr uint64_t C = 1442695040888963407ULL;
        return seed * A + C;
    }

    /**
     * @brief 個体のチェックサム
     */
    uint64_t checksum_value;
};

}