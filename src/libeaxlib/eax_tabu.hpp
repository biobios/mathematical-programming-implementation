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
class EAX_tabu {
public:
    EAX_tabu(size_t city_size);
    EAX_tabu(ObjectPools& object_pools)
        : vector_of_tsp_size_pool(object_pools.vector_of_tsp_size_pool.share()),
          any_size_vector_pool(object_pools.any_size_vector_pool.share()),
          intermediate_individual_pool(object_pools.intermediate_individual_pool.share()),
          ab_cycle_finder(object_pools),
          subtour_merger(object_pools) {}
    
    EAX_tabu(
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
    
    enum class SelectionMethod {
        EAX_UNIFORM,
        EAX_half_UNIFORM,
        EAX_1AB,
        EAX_Rand,
    };

    /**
     * @brief タブー制約付き交叉
     * @param parent1 親個体1
     * @param parent2 親個体2
     * @param children_size 生成する子個体の数
     * @param tabu_edges タブーエッジのリスト
     * @param tsp 問題の情報
     * @param rng 乱数生成器
     * @param selection_method 使用するABサイクル選択方法
     * @return 生成した子個体の変更履歴のリスト
     * @tparam Individual 双方向連結リストのようなインターフェースを持つ個体の型
     */
    template <doubly_linked_list_like Individual>
    std::vector<CrossoverDelta> operator()(const Individual& parent1, const Individual& parent2, size_t children_size,
                                        std::vector<std::pair<size_t, size_t>> const& tabu_edges, const tsp::TSP& tsp, std::mt19937& rng, SelectionMethod selection_method) {
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

        auto AB_cycles = ab_cycle_finder(std::numeric_limits<size_t>::max(), parent1, parent2, rng);
        
        remove_tabu_AB_cycles(AB_cycles, tabu_edges);

        auto working_individual = intermediate_individual_pool.acquire_unique();
        working_individual->assign(parent1);

        switch (selection_method) {
            case SelectionMethod::EAX_UNIFORM:
                return generate_children_via_EAX_UNIFORM(AB_cycles, children_size, *working_individual, tsp, rng);
            case SelectionMethod::EAX_half_UNIFORM:
                return generate_children_via_EAX_half_UNIFORM(AB_cycles, children_size, *working_individual, tsp, rng);
            case SelectionMethod::EAX_1AB:
                return generate_children_via_EAX_1AB(AB_cycles, children_size, *working_individual, tsp, rng);
            case SelectionMethod::EAX_Rand:
                return generate_children_via_EAX_Rand(AB_cycles, children_size, *working_individual, tsp, rng);
            default:
                throw std::invalid_argument("Invalid selection method");
        }
    }
    
private:
    // タブーエッジを含むABサイクルを削除する
    void remove_tabu_AB_cycles(std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles,
                               const std::vector<std::pair<size_t, size_t>>& tabu_edges);
    std::vector<CrossoverDelta> generate_children_via_EAX_UNIFORM(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles,
                                                                        size_t children_size,
                                                                        IntermediateIndividual& working_individual,
                                                                        const tsp::TSP& tsp,
                                                                        std::mt19937& rng);
    std::vector<CrossoverDelta> generate_children_via_EAX_half_UNIFORM(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles,
                                                                        size_t children_size,
                                                                        IntermediateIndividual& working_individual,
                                                                        const tsp::TSP& tsp,
                                                                        std::mt19937& rng);
    std::vector<CrossoverDelta> generate_children_via_EAX_1AB(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles,
                                                                        size_t children_size,
                                                                        IntermediateIndividual& working_individual,
                                                                        const tsp::TSP& tsp,
                                                                        std::mt19937& rng);
    std::vector<CrossoverDelta> generate_children_via_EAX_Rand(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles,
                                                                        size_t children_size,
                                                                        IntermediateIndividual& working_individual,
                                                                        const tsp::TSP& tsp,
                                                                        std::mt19937& rng);
    mpi::ObjectPool<std::vector<size_t>> vector_of_tsp_size_pool;
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
    mpi::ObjectPool<IntermediateIndividual> intermediate_individual_pool;
    ABCycleFinder ab_cycle_finder;
    SubtourMerger subtour_merger;
};
}