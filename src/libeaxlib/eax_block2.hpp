#pragma once

#include <vector>
#include <memory>
#include <random>
#include <ranges>

#include "eaxdef.hpp"
#include "object_pools.hpp"
#include "crossover_delta.hpp"
#include "tsp_loader.hpp"
#include "ab_cycle_finder.hpp"
#include "block2_e_set_assembler.hpp"
#include "subtour_merger.hpp"

namespace eax {
class EAX_Block2 {
public:
    EAX_Block2(ObjectPools& object_pools)
        : vector_of_tsp_size_pool(object_pools.vector_of_tsp_size_pool.share()),
          any_size_vector_pool(object_pools.any_size_vector_pool.share()),
          intermediate_individual_pool(object_pools.intermediate_individual_pool.share()),
          ab_cycle_finder(object_pools),
          block2_e_set_assembler_builder(object_pools),
          subtour_merger(object_pools) {}

    /**
     * @brief 交叉操作を実行する
     * @param parent1 親個体1
     * @param parent2 親個体2
     * @param children_size 生成する子個体の数
     * @param tsp TSPインスタンス
     * @param rng 乱数生成器
     * @return 生成された子個体の変更履歴のベクター
     */
    std::vector<CrossoverDelta> operator()(const individual_readable auto& parent1, const individual_readable auto& parent2, size_t children_size,
                                        const tsp::TSP& tsp, std::mt19937& rng) {
        auto& adjacency_matrix = tsp.adjacency_matrix;
        using namespace std;

        const size_t n = parent1.size();

        auto path_ptr = vector_of_tsp_size_pool.acquire_unique();
        auto pos_ptr = vector_of_tsp_size_pool.acquire_unique();
        vector<size_t>& path = *path_ptr;
        vector<size_t>& pos = *pos_ptr;
        
        // 親間で異なる枝の本数
        size_t different_edges_count = 0;
        for (size_t i = 0, prev = 0, current = 0; i < n; ++i) {
            path[i] = current;
            pos[current] = i;
            size_t next = parent1[current][0];
            if (next == prev) {
                next = parent1[current][1];
            }
            
            if (parent2[current][0] != next && parent2[current][1] != next)
                ++different_edges_count;
            prev = current;
            current = next;
        }

        auto AB_cycles = ab_cycle_finder(numeric_limits<size_t>::max(), parent1, parent2, rng);
        
        sort(AB_cycles.begin(), AB_cycles.end(), [](const mpi::pooled_unique_ptr<ab_cycle_t>& a, const mpi::pooled_unique_ptr<ab_cycle_t>& b) {
            return a->size() > b->size();
        });
        
        auto block2_e_set_assembler = block2_e_set_assembler_builder.create(parent1, parent2, AB_cycles);
        
        children_size = min(children_size, AB_cycles.size());

        vector<CrossoverDelta> children;
        auto working_individual = intermediate_individual_pool.acquire_unique();
        working_individual->assign(parent1);
        for (size_t child_index = 0; child_index < children_size; ++child_index) {
            auto selected_AB_cycles_indices_ptr = block2_e_set_assembler(child_index, rng);
            auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
            
            if (selected_AB_cycles_indices.size() == AB_cycles.size()) {
                continue; // 全てのABサイクルを選択している場合はスキップ
            }
            
            auto selected_AB_cycles_view = selected_AB_cycles_indices | views::transform([&AB_cycles](size_t index) -> const ab_cycle_t& {
                return *AB_cycles[index];
            });

            working_individual->apply_AB_cycles(selected_AB_cycles_view);

            subtour_merger(*working_individual, tsp, selected_AB_cycles_view);
            
            // 削除された親1の枝の数(追加された親２の枝の数)
            size_t swapped_edges_count = 0;
            for (size_t i = 0; i < selected_AB_cycles_indices.size(); ++i) {
                const auto& cycle = *AB_cycles[selected_AB_cycles_indices[i]];
                swapped_edges_count += cycle.size() / 2;
            }
            
            if (swapped_edges_count * 2 >= different_edges_count &&
                parent1.get_distance() + working_individual->calc_delta_distance(adjacency_matrix) == parent2.get_distance()) {
                // 交換された枝の数が親間で異なる枝の数の半分以上であり、
                // 子供の距離が親2と同じ場合は、子供を追加しない
                working_individual->discard();
                continue;
            }

            children.emplace_back(working_individual->get_delta_and_revert(adjacency_matrix));
        }
        return children;
    }
private:
    mpi::ObjectPool<std::vector<size_t>> vector_of_tsp_size_pool;
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
    mpi::ObjectPool<IntermediateIndividual> intermediate_individual_pool;
    ABCycleFinder ab_cycle_finder;
    Block2ESetAssemblerBuilder block2_e_set_assembler_builder;
    SubtourMerger subtour_merger;
};

class EAX_Block2_tag {
public:
    EAX_Block2_tag() = default;
    EAX_Block2_tag(const std::string& str) {
        if (!match_string(str)) {
            throw std::invalid_argument("Invalid EAX type string for EAX_Block2.");
        }
    }
    static bool match_string(const std::string& str) {
        return str == "Block2";
    }
    std::string to_string() const {
        return "Block2";
    }
};
}