#pragma once

#include <vector>
#include <cstdint>
#include <random>

namespace eax {
    /**
     * @brief 2-opt局所探索を表すクラス
     */
    class TwoOpt {
    public:
        /**
         * @brief 指定した距離行列と近傍リストで2-opt局所探索を初期化する
         * @param distance_matrix 距離行列
         * @param nearest_neighbors 近傍リスト
         * @param near_range 近傍範囲
         */
        TwoOpt(const std::vector<std::vector<int64_t>>& distance_matrix,
               const std::vector<std::vector<std::pair<int64_t, size_t>>>& nearest_neighbors,
               size_t near_range = 50);
        
        /**
         * @brief 指定した巡回路に対して2-opt局所探索を適用する
         * @param path 巡回路を表す頂点のベクター
         * @param seed 乱数シード
         */
        void apply(std::vector<size_t>& path, std::mt19937::result_type seed = std::mt19937::default_seed);

    private:
        std::vector<std::vector<int64_t>> distance_matrix;
        std::vector<std::vector<std::pair<int64_t, size_t>>> nearest_neighbors;
        std::vector<std::vector<size_t>> near_cities;
        const size_t near_range;
    };
    
    void print_2opt_time();
}