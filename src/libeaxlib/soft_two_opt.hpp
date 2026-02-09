#pragma once

#include "tsp_loader.hpp"

#include <vector>
#include <cstdint>

namespace eax {

// TODO: SoftTwoOptクラスをここに追加
// 旧eaxのtwo_optをベースに実装
class SoftTwoOpt {
public:
    SoftTwoOpt(const tsp::adjacency_matrix_t& distance_matrix,
                const tsp::NN_list_t& nearest_neighbors,
                size_t near_range = 20);
    
    void apply(std::vector<size_t>& path);

private:
    tsp::adjacency_matrix_t distance_matrix;
    tsp::NN_list_t nearest_neighbors;
    const size_t near_range;
};
}