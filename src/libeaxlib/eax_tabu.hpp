#pragma once

#include <vector>
#include <memory>
#include <random>
#include <ranges>

#include "object_pool.hpp"

#include "eaxdef.hpp"
#include "object_pools.hpp"
#include "crossover_delta.hpp"
#include "tsp_loader.hpp"
#include "ab_cycle_finder.hpp"
#include "subtour_merger.hpp"
#include "intermediate_individual.hpp"

#include "eax_rand.hpp"
#include "eax_n_ab.hpp"

namespace eax {
template <typename E_Set_Assembler_Builder>
class EAX_tabu {
public:
    EAX_tabu(ObjectPools& object_pools)
        : intermediate_individual_pool(object_pools.intermediate_individual_pool.share()),
          vector_of_tsp_size_pool(object_pools.vector_of_tsp_size_pool.share()),
          ab_cycle_finder(object_pools),
          subtour_merger(object_pools),
          e_set_assembler_builder(object_pools) {}

    template <doubly_linked_list_like Individual, typename... Args>
    std::vector<CrossoverDelta> operator()(const Individual& parent1, const Individual& parent2, size_t children_size,
                                        std::vector<std::pair<size_t, size_t>> const& tabu_edges, const tsp::TSP& tsp, std::mt19937& rng, Args&&... args) {
        using namespace std;
        auto AB_cycles = ab_cycle_finder(std::numeric_limits<size_t>::max(), parent1, parent2, rng);
        
        remove_tabu_AB_cycles(AB_cycles, tabu_edges);
        
        auto e_set_assembler = e_set_assembler_builder.build(AB_cycles, parent1, parent2, children_size, tsp, rng, forward<Args>(args)...);
        
        std::vector<CrossoverDelta> children;
        
        auto working_individual_ptr = intermediate_individual_pool.acquire_unique();
        IntermediateIndividual& working_individual = *working_individual_ptr;
        working_individual.assign(parent1);
        
        for (size_t i = 0; i < children_size && e_set_assembler.has_next(); ++i) {
            auto e_set_indices_ptr = e_set_assembler.next(rng);
            auto& e_set_indices = *e_set_indices_ptr;
 
            auto selected_AB_cycles_view = std::views::transform(e_set_indices, [&AB_cycles](size_t index) -> const ab_cycle_t& {
                return *AB_cycles[index];
            }); 

            working_individual.apply_AB_cycles(selected_AB_cycles_view);
            subtour_merger(working_individual, tsp, selected_AB_cycles_view);

            children.emplace_back(working_individual.get_delta_and_revert());
        }
        
        return children;
    }
private:
    mpi::ObjectPool<IntermediateIndividual> intermediate_individual_pool;
    mpi::ObjectPool<std::vector<size_t>> vector_of_tsp_size_pool;
    ABCycleFinder ab_cycle_finder;
    SubtourMerger subtour_merger;
    E_Set_Assembler_Builder e_set_assembler_builder;

    // タブーエッジを含むABサイクルを削除する
    void remove_tabu_AB_cycles(std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles,
                                const std::vector<std::pair<size_t, size_t>>& tabu_edges) {
        // 一つの都市は最大2つのABサイクルに含まれるので、vector_of_tsp_sizeを2つ用意する
        auto in_cycle1_ptr = vector_of_tsp_size_pool.acquire_unique();
        auto in_cycle2_ptr = vector_of_tsp_size_pool.acquire_unique();
        std::vector<size_t>& in_cycle1 = *in_cycle1_ptr;
        std::vector<size_t>& in_cycle2 = *in_cycle2_ptr;
        const size_t null_cycle = AB_cycles.size();
        in_cycle1.assign(in_cycle1.size(), null_cycle);
        in_cycle2.assign(in_cycle2.size(), null_cycle);

        for (size_t i = 0; i < AB_cycles.size(); ++i) {
            const ab_cycle_t& cycle = *AB_cycles[i];
            for (size_t city : cycle) {
                if (in_cycle1[city] == null_cycle) {
                    in_cycle1[city] = i;
                } else {
                    in_cycle2[city] = i;
                }
            }
        }
        
        // タブーエッジを含むABサイクルを削除する
        for (const auto& [v1, v2] : tabu_edges) {
            size_t cycle_v1_1 = in_cycle1[v1];
            size_t cycle_v1_2 = in_cycle2[v1];
            size_t cycle_v2_1 = in_cycle1[v2];
            size_t cycle_v2_2 = in_cycle2[v2];
            if ((cycle_v1_1 == cycle_v2_1) || (cycle_v1_1 == cycle_v2_2)) {
                if (cycle_v1_1 != null_cycle) {
                    AB_cycles[cycle_v1_1].reset();
                }
            }
            if ((cycle_v1_2 == cycle_v2_1) || (cycle_v1_2 == cycle_v2_2)) {
                if (cycle_v1_2 != null_cycle) {
                    AB_cycles[cycle_v1_2].reset();
                }
            }
        }
        
        // nullptrになった要素を削除する
        for (size_t i = AB_cycles.size(); i > 0; --i) {
            if (AB_cycles[i - 1] == nullptr) {
                if (i - 1 != AB_cycles.size() - 1) {
                    std::swap(AB_cycles[i - 1], AB_cycles.back());
                }
                AB_cycles.pop_back();
            }
        }
    }
};
using EAX_tabu_Rand = EAX_tabu<Rand_e_set_assembler_builder>;
using EAX_tabu_N_AB = EAX_tabu<N_AB_e_set_assembler_builder>;
}