#pragma once

#include <random>
#include <ranges>

#include "object_pool.hpp"

#include "object_pools.hpp"
#include "tsp_loader.hpp"
#include "eaxdef.hpp"
#include "eax_normal.hpp"

namespace eax {
class Rand_e_set_assembler {
public:
    Rand_e_set_assembler(size_t ab_cycle_count, mpi::ObjectPool<std::vector<size_t>>&& any_size_vector_pool)
        : ab_cycle_count(ab_cycle_count), any_size_vector_pool(std::move(any_size_vector_pool)) {}
    
    bool has_next() const {
        return ab_cycle_count > 0;
    }
    
    mpi::pooled_unique_ptr<std::vector<size_t>> next(std::mt19937& rng) {
        auto selected_AB_cycles_indices_ptr = any_size_vector_pool.acquire_unique();
        auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
        selected_AB_cycles_indices.clear();
        std::uniform_int_distribution<size_t> dist_01(0, 1);
        for (size_t i = 0; i < ab_cycle_count; ++i) {
            if (dist_01(rng) == 0) {
                selected_AB_cycles_indices.push_back(i);
            }
        }
        return selected_AB_cycles_indices_ptr;
    }

private:
    size_t ab_cycle_count;
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
};

class Rand_e_set_assembler_builder {
public:
    Rand_e_set_assembler_builder(ObjectPools& object_pools) :
        any_size_vector_pool(object_pools.any_size_vector_pool.share()) {}
    
    template <doubly_linked_list_like Individual>
    Rand_e_set_assembler build(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles, [[maybe_unused]]const Individual& parent1, [[maybe_unused]]const Individual parent2, [[maybe_unused]]size_t children_size, [[maybe_unused]]const tsp::TSP& tsp, [[maybe_unused]]std::mt19937& rng) {
        return Rand_e_set_assembler(AB_cycles.size(), any_size_vector_pool.share());
    }

    static size_t calc_AB_cycle_need([[maybe_unused]]const auto& parent1, [[maybe_unused]]const auto& parent2, [[maybe_unused]]size_t children_size, [[maybe_unused]]const tsp::TSP& tsp, [[maybe_unused]]std::mt19937& rng) {
        return std::numeric_limits<size_t>::max();
    }
private:
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
};

using EAX_Rand = EAX_normal<Rand_e_set_assembler_builder>;

class EAX_Rand_tag {
public:
    EAX_Rand_tag() {}
    EAX_Rand_tag(const std::string& str) {}
    static bool match_string(const std::string& str) {
        return str == "EAX_Rand";
    }
    std::string to_string() const {
        return "EAX_Rand";
    }
};
}