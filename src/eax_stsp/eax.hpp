#pragma once

#include <vector>
#include <random>

#include "environment.hpp"

namespace eax {
    
    std::vector<std::vector<size_t>> edge_assembly_crossover(const std::vector<size_t>& parent1, const std::vector<size_t>& parent2, size_t children_size,
                                            eax::Environment& env, std::mt19937& rng);

    void print_time();
}