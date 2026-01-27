#pragma once

#include <vector>
#include <cstdint>

namespace eax {

// TODO: SoftTwoOptクラスをここに追加
// 旧eaxのtwo_optをベースに実装
class SoftTwoOpt {
public:
    SoftTwoOpt(const std::vector<std::vector<int64_t>>& distance_matrix,
                const std::vector<std::vector<std::pair<int64_t, size_t>>>& nearest_neighbors,
                size_t near_range = 20);
    
    void apply(std::vector<size_t>& path);

private:
    std::vector<std::vector<int64_t>> distance_matrix;
    std::vector<std::vector<std::pair<int64_t, size_t>>> nearest_neighbors;
    const size_t near_range;
};
}