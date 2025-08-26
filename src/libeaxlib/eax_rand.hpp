#pragma once

#include <random>
#include <memory>
#include <ranges>

#include "object_pool.hpp"

#include "object_pools.hpp"
#include "tsp_loader.hpp"
#include "eaxdef.hpp"
#include "crossover_delta.hpp"
#include "intermediate_individual.hpp"
#include "ab_cycle_finder.hpp"
#include "subtour_merger.hpp"

namespace eax {
class EAX_Rand {
public:
    EAX_Rand(size_t city_size);
    
    EAX_Rand(ObjectPools& object_pools)
        : vector_of_tsp_size_pool(object_pools.vector_of_tsp_size_pool.share()),
          any_size_vector_pool(object_pools.any_size_vector_pool.share()),
          intermediate_individual_pool(object_pools.intermediate_individual_pool.share()),
          ab_cycle_finder(object_pools),
          subtour_merger(object_pools) {}

    EAX_Rand(
        mpi::ObjectPool<std::vector<size_t>> vector_of_tsp_size_pool,
        mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool,
        mpi::ObjectPool<IntermediateIndividual> intermediate_individual_pool,
        ABCycleFinder ab_cycle_finder,
        SubtourMerger subtour_merger)
        : vector_of_tsp_size_pool(std::move(vector_of_tsp_size_pool)),
          any_size_vector_pool(std::move(any_size_vector_pool)),
          intermediate_individual_pool(std::move(intermediate_individual_pool)),
          ab_cycle_finder(std::move(ab_cycle_finder)),
          subtour_merger(std::move(subtour_merger)) {}

    template <doubly_linked_list_like Individual>
    std::vector<CrossoverDelta> operator()(const Individual& parent1, const Individual& parent2, size_t children_size,
                                        const tsp::TSP& tsp, std::mt19937& rng) {
        using namespace std;

        const size_t n = parent1.size();

        auto path_ptr = vector_of_tsp_size_pool.acquire_unique();
        auto pos_ptr = vector_of_tsp_size_pool.acquire_unique();
        vector<size_t>& path = *path_ptr;
        vector<size_t>& pos = *pos_ptr;

        for (size_t i = 0, prev = 0, current = 0; i < n; ++i) {
            path[i] = current;
            pos[current] = i;
            size_t next = parent1[current][0];
            if (next == prev) {
                next = parent1[current][1];
            }
            prev = current;
            current = next;
        }

        auto AB_cycles = ab_cycle_finder(numeric_limits<size_t>::max(), parent1, parent2, rng);

        vector<CrossoverDelta> children;
        auto working_individual = intermediate_individual_pool.acquire_unique();
        working_individual->assign(parent1);

        for (size_t child_index = 0; child_index < children_size; ++child_index) {

            // 緩和個体を作成
            auto selected_AB_cycles_indices_ptr = any_size_vector_pool.acquire_unique();
            auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
            selected_AB_cycles_indices.clear();

            uniform_int_distribution<size_t> dist_01(0, 1);
            for (size_t i = 0; i < AB_cycles.size(); ++i) {
                if (dist_01(rng) == 0) {
                    selected_AB_cycles_indices.push_back(i);
                }
            }

            auto selected_AB_cycles_view = selected_AB_cycles_indices | views::transform([&AB_cycles](size_t index) -> const ab_cycle_t& {
                return *AB_cycles[index];
            });

            working_individual->apply_AB_cycles(selected_AB_cycles_view);

            subtour_merger(*working_individual, tsp, selected_AB_cycles_view);

            children.emplace_back(working_individual->get_delta_and_revert());

        }
        if (children.empty()) {
            children.emplace_back(working_individual->get_delta_and_revert());
        }
        return children;
    }

private:
    mpi::ObjectPool<std::vector<size_t>> vector_of_tsp_size_pool;
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
    mpi::ObjectPool<IntermediateIndividual> intermediate_individual_pool;
    ABCycleFinder ab_cycle_finder;
    SubtourMerger subtour_merger;
};
}