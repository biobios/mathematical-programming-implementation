#pragma once

#include <vector>
#include <random>

#include "individual.hpp"
#include "tsp_loader.hpp"
#include "environment.hpp"

namespace eax {
    
    std::vector<Child> edge_assembly_crossover(const Individual& parent1, const Individual& parent2, size_t children_size,
                                            const Environment& env, std::mt19937& rng);

    void print_time();
}