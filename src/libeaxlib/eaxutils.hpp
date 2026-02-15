#pragma once

#include "eaxdef.hpp"

#include <iostream>

namespace eax {

template <individual_readable Individual>
void print_best_solution(const std::vector<Individual>& population, std::ostream& os) {
    size_t best_index = 0;
    int64_t best_length = population[0].get_distance();
    for (size_t i = 1; i < population.size(); ++i) {
        int64_t length = population[i].get_distance();
        if (length < best_length) {
            best_length = length;
            best_index = i;
        }
    }
    
    auto& best_ind = population[best_index];

    std::vector<size_t> best_path;
    size_t prev = 0;
    size_t current = 0;
    for (size_t i = 0; i < best_ind.size(); ++i) {
        best_path.push_back(current);
        size_t next = best_ind[current][0];
        if (next == prev) {
            next = best_ind[current][1];
        }
        prev = current;
        current = next;
    }
    
    os << "Best Solution: " << best_length << std::endl;
    os << "Best Path: ";
    for (size_t city : best_path) {
        os << city << " ";
    }
    os << std::endl;
}

}