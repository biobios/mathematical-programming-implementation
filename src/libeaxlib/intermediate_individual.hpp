#pragma once

#include <limits>

#include "eaxdef.hpp"
#include "crossover_delta.hpp"

namespace eax {
class IntermediateIndividual {
public:
    IntermediateIndividual(size_t size);

    template <doubly_linked_list_like T>
    IntermediateIndividual(const T& individual)
        : individual_being_edited(individual.size()),
        modification_pool(),
        modifications(modification_pool.acquire()),
        path(individual.size()),
        pos(individual.size()) {
        assign(individual);
    }

    template <doubly_linked_list_like T>
    void assign(const T& individual) {
        reset();
        for (size_t i = 0; i < individual.size(); ++i) {
            individual_being_edited[i] = {individual[i][0], individual[i][1]};
        }

        size_t prev = 0;
        size_t current = 0;
        for (size_t i = 0; i < individual.size(); ++i) {
            path[i] = current;
            pos[current] = i;

            size_t next = individual[current][0];
            if (next == prev) {
                next = individual[current][1];
            }

            prev = current;
            current = next;
        }
    }
    
    CrossoverDelta get_delta_and_revert();
    void discard();
    
    const std::array<size_t, 2>& operator[](size_t index) const;
    
    void change_connection(size_t v1, size_t v2, size_t new_v2);
    void swap_edges(std::pair<size_t, size_t> edge1, std::pair<size_t, size_t> edge2);

    template <std::ranges::range ABCycles>
        requires std::convertible_to<std::ranges::range_value_t<ABCycles>, const ab_cycle_t&>
    void apply_AB_cycles(const ABCycles& AB_cycles) {
        using namespace std;

        auto edge_swap = [this](size_t b1, size_t ba, size_t ab, size_t b2) {
            change_connection(ba, ab, b1);
            change_connection(ab, ba, b2);
        };

        for (const ab_cycle_t& cycle : AB_cycles) {
            for (size_t i = 2; i < cycle.size() - 2; i += 2) {
                edge_swap(cycle[i - 1], cycle[i], cycle[i + 1], cycle[i + 2]);
            }
            // i = 0
            {
                edge_swap(cycle[cycle.size() - 1], cycle[0], cycle[1], cycle[2]);
            }
            // i = cycle.size() - 2
            {
                edge_swap(cycle[cycle.size() - 3], cycle[cycle.size() - 2], cycle[cycle.size() - 1], cycle[0]);
            }
        }
    }

    const std::vector<size_t>& get_path() const;
    const std::vector<size_t>& get_pos() const;
    int64_t calc_delta_distance(const std::vector<std::vector<int64_t>>& adjacency_matrix) const;
    size_t size() const;
private:
    void revert();
    void reset();
    void undo(const CrossoverDelta::Modification& modification);
    doubly_linked_list_t individual_being_edited;
    mpi::ObjectPool<std::vector<CrossoverDelta::Modification>> modification_pool;
    mpi::pooled_ptr<std::vector<CrossoverDelta::Modification>> modifications;
    std::vector<size_t> path;
    std::vector<size_t> pos;
};
}