#pragma once

#include <vector>
#include <cstdint>
#include <random>

namespace eax {
    class TwoOpt {
    public:
        TwoOpt(const std::vector<std::vector<int64_t>>& distance_matrix,
               const std::vector<std::vector<std::pair<int64_t, size_t>>>& nearest_neighbors,
               size_t near_range = 50);
        
        // 2-optの適用
        void apply(std::vector<size_t>& path, std::mt19937::result_type seed = std::mt19937::default_seed);

    private:
        std::vector<std::vector<int64_t>> distance_matrix;
        std::vector<std::vector<std::pair<int64_t, size_t>>> nearest_neighbors;
        std::vector<std::vector<size_t>> near_cities;
        const size_t near_range;
    };
    
    void print_2opt_time();
}