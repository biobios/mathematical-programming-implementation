#pragma once

#include <vector>
#include <random>

namespace eax {
    
    std::vector<std::vector<size_t>> edge_assembly_crossover(const std::vector<size_t>& parent1, const std::vector<size_t>& parent2, size_t children_size,
                                            const std::vector<std::vector<int64_t>>& adjacency_matrix, std::mt19937& rng);
    
    void print_time();
}