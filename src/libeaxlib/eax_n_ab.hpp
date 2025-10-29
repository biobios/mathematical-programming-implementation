#pragma once

#include <vector>
#include <memory>
#include <random>
#include <ranges>

#include "object_pool.hpp"

#include "eaxdef.hpp"
#include "object_pools.hpp"
#include "tsp_loader.hpp"
#include "eax_normal.hpp"

namespace eax {
class N_AB_e_set_assembler {
public:
    N_AB_e_set_assembler(size_t ab_cycle_count, size_t N_parameter, mpi::ObjectPool<std::vector<size_t>>&& any_size_vector_pool, std::mt19937& rng)
        : ab_cycle_count(ab_cycle_count),
            N_parameter(N_parameter),
            any_size_vector_pool(std::move(any_size_vector_pool)),
            indices_of_remaining_AB_cycles(this->any_size_vector_pool.acquire_unique()) {

        auto& indices = *indices_of_remaining_AB_cycles;
        indices.clear();
        indices.resize(ab_cycle_count);

        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);
    }
    
    bool has_next() const {
        if (N_parameter == 1 || ab_cycle_count <= N_parameter) {
            // シャッフルしなおしても必ず同じ組み合わせになる場合、
            // 残りのサイクルを使いきったら終了
            return indices_of_remaining_AB_cycles->size() > 0;
        }
        
        // シャッフルしなおすことで別の組み合わせが生成可能な場合、常に真
        return true;
    }
    
    mpi::pooled_unique_ptr<std::vector<size_t>> next(std::mt19937& rng) {
        auto& indices = *indices_of_remaining_AB_cycles;
        
        if (indices.size() < N_parameter && ab_cycle_count > N_parameter && N_parameter > 1) {
            // 残りのサイクルがN_parameter未満で、かつ、シャッフルしなおすことで別の組み合わせを生成可能な場合
            indices.resize(ab_cycle_count);
            std::iota(indices.begin(), indices.end(), 0);
            std::shuffle(indices.begin(), indices.end(), rng);
        }

        auto selected_AB_cycles_indices_ptr = any_size_vector_pool.acquire_unique();
        auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
        selected_AB_cycles_indices.clear();

        for (size_t i = 0; i < N_parameter && !indices.empty(); ++i) {
            size_t index = indices.back();
            indices.pop_back();
            
            selected_AB_cycles_indices.push_back(index);
        }
        return selected_AB_cycles_indices_ptr;
    }
private:
    size_t ab_cycle_count;
    size_t N_parameter;
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;

    mpi::pooled_unique_ptr<std::vector<size_t>> indices_of_remaining_AB_cycles;
};

class N_AB_e_set_assembler_builder {
public:
    N_AB_e_set_assembler_builder(ObjectPools& object_pools) :
        any_size_vector_pool(object_pools.any_size_vector_pool.share()) {}

    template <doubly_linked_list_like Individual>
    N_AB_e_set_assembler build(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles, [[maybe_unused]]const Individual& parent1, [[maybe_unused]]const Individual parent2, [[maybe_unused]]size_t children_size, [[maybe_unused]]const tsp::TSP& tsp, [[maybe_unused]]std::mt19937& rng, size_t N_parameter) {
        return N_AB_e_set_assembler(AB_cycles.size(), N_parameter, any_size_vector_pool.share(), rng);
    }

    static size_t calc_AB_cycle_need([[maybe_unused]]const auto& parent1, [[maybe_unused]]const auto& parent2, size_t children_size, [[maybe_unused]]const tsp::TSP& tsp, [[maybe_unused]]std::mt19937& rng, size_t N_parameter) {
        return N_parameter * children_size;
    }
private:
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
};

using EAX_N_AB = EAX_normal<N_AB_e_set_assembler_builder>;
}