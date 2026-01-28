#pragma once

#include <vector>
#include <ranges>

#include "utils.hpp"
#include "object_pool.hpp"

#include "eaxdef.hpp"
#include "intermediate_individual.hpp"
#include "crossover_delta.hpp"
#include "object_pools.hpp"
#include "ab_cycle_finder.hpp"
#include "subtour_merger.hpp"

namespace eax {
/**
 * @brief 基本的なEAX交叉を行うクラス
 * @tparam E_Set_Assembler_Builder E_Set_Assemblerを構築するクラス
 * @tparam Subtour_Merger 部分巡回路を統合するクラス
 */
template <typename E_Set_Assembler_Builder, typename Subtour_Merger = SubtourMerger, typename AB_Cycle_Finder = ABCycleFinder>
    requires (E_Set_Assembler_Builder::is_available(typename AB_Cycle_Finder::completeness_category{}))
class EAX_normal {
public:
    EAX_normal(ObjectPools& object_pools)
        : intermediate_individual_pool(object_pools.intermediate_individual_pool.share()),
          ab_cycle_finder(object_pools),
          subtour_merger(object_pools),
          e_set_assembler_builder(object_pools) {}

    template <typename BuilderArgsTuple = std::tuple<>, typename MergerArgsTuple = std::tuple<>, typename FinderArgsTuple = std::tuple<>>
    std::vector<CrossoverDelta> operator()(const individual_readable auto& parent1, const individual_readable auto& parent2, size_t children_size, const tsp::TSP& tsp, std::mt19937& rng,
                                            BuilderArgsTuple&& builder_args = {}, MergerArgsTuple&& merger_args = {}, FinderArgsTuple&& finder_args = {}) {
        using namespace std;
        
        size_t ab_cycle_need = [&]() {
            if constexpr (mpi::tuple_like<BuilderArgsTuple>) {
                // BuilderArgsTupleがtuple_likeであれば引数を展開して渡す
                return std::apply(
                    [&](auto&&... args) {
                        return E_Set_Assembler_Builder::calc_AB_cycle_need(parent1, parent2, children_size, tsp, rng, args...);
                    }, builder_args
                );
            } else {
                // そうでなければそのまま渡す
                return E_Set_Assembler_Builder::calc_AB_cycle_need(parent1, parent2, children_size, tsp, rng, builder_args);
            }
        }();
        
        auto AB_cycles = [&]() {
            if constexpr (mpi::tuple_like<FinderArgsTuple>) {
                // FinderArgsTupleがtuple_likeであれば引数を展開して渡す
                return std::apply(
                    [&](auto&&... args) {
                        return ab_cycle_finder(ab_cycle_need, parent1, parent2, rng, std::forward<decltype(args)>(args)...);
                    }, std::forward<FinderArgsTuple>(finder_args)
                );
            } else {
                // そうでなければそのまま渡す
                return ab_cycle_finder(ab_cycle_need, parent1, parent2, rng, std::forward<FinderArgsTuple>(finder_args));
            }
        }();

        auto e_set_assembler = [&]() {
            if constexpr (mpi::tuple_like<BuilderArgsTuple>) {
                // BuilderArgsTupleがtuple_likeであれば引数を展開して渡す
                return std::apply(
                    [&](auto&&... args) {
                        return e_set_assembler_builder.build(AB_cycles, parent1, parent2, children_size, tsp, rng, std::forward<decltype(args)>(args)...);
                    }, std::forward<BuilderArgsTuple>(builder_args)
                );
            } else {
                // そうでなければそのまま渡す
                return e_set_assembler_builder.build(AB_cycles, parent1, parent2, children_size, tsp, rng, std::forward<BuilderArgsTuple>(builder_args));
            }
        }();
        
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
            
            if constexpr (mpi::tuple_like<MergerArgsTuple>) {
                // MergerArgsTupleがtuple_likeであれば引数を展開して渡す
                std::apply(
                    [&](auto&&... args) {
                        subtour_merger(working_individual, tsp, selected_AB_cycles_view, std::forward<decltype(args)>(args)...);
                    }, std::forward<MergerArgsTuple>(merger_args)
                );
            } else {
                // そうでなければそのまま渡す
                subtour_merger(working_individual, tsp, selected_AB_cycles_view, std::forward<MergerArgsTuple>(merger_args));
            }

            children.emplace_back(working_individual.get_delta_and_revert(tsp.adjacency_matrix));
        }
        
        return children;
    }
private:
    mpi::ObjectPool<IntermediateIndividual> intermediate_individual_pool;
    AB_Cycle_Finder ab_cycle_finder;
    Subtour_Merger subtour_merger;
    E_Set_Assembler_Builder e_set_assembler_builder;
};
}