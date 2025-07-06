#pragma once

#include <vector>
#include <random>

#include "tsp_loader.hpp"

namespace eax {
    
    std::vector<std::vector<size_t>> edge_assembly_crossover(const std::vector<size_t>& parent1, const std::vector<size_t>& parent2, size_t children_size,
                                            const tsp::TSP& tsp, std::mt19937& rng);

    void print_time();
}