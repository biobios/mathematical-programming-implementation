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

namespace eax {
class EAX_N_AB {
public:
    EAX_N_AB(size_t city_size);
    EAX_N_AB(ObjectPools& object_pools)
        : vector_of_tsp_size_pool(object_pools.vector_of_tsp_size_pool.share()),
          any_size_vector_pool(object_pools.any_size_vector_pool.share()),
          intermediate_individual_pool(object_pools.intermediate_individual_pool.share()),
          ab_cycle_finder(object_pools),
          subtour_merger(object_pools) {}
    
    EAX_N_AB(
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
                                        const tsp::TSP& tsp, std::mt19937& rng, size_t N_parameter = 1) {
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

        auto AB_cycles = ab_cycle_finder(N_parameter * children_size, parent1, parent2, rng);

        auto AB_cycle_indices_ptr = any_size_vector_pool.acquire_unique();
        auto& AB_cycle_indices = *AB_cycle_indices_ptr;
        AB_cycle_indices.resize(AB_cycles.size());
        iota(AB_cycle_indices.begin(), AB_cycle_indices.end(), 0);
        shuffle(AB_cycle_indices.begin(), AB_cycle_indices.end(), rng);

        vector<CrossoverDelta> children;
        auto working_individual = intermediate_individual_pool.acquire_unique();
        working_individual->assign(parent1);

        children_size = min(children_size, (AB_cycles.size() + N_parameter - 1) / N_parameter);

        for (size_t child_index = 0; child_index < children_size; ++child_index) {

            // 緩和個体を作成
            auto selected_AB_cycles_indices_ptr = any_size_vector_pool.acquire_unique();
            auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
            selected_AB_cycles_indices.clear();

            for (size_t i = 0; i < N_parameter && i < AB_cycles.size(); ++i) {
                size_t index = (child_index * N_parameter + i) % AB_cycles.size();
                selected_AB_cycles_indices.push_back(AB_cycle_indices[index]);
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