#pragma once

#include <memory>
#include <tuple>

#include "object_pool.hpp"

#include "object_pools.hpp"
#include "intermediate_individual.hpp"
#include "subtour_list.hpp"

namespace eax {
class SubtourFinder {
public:
    using subtour_list_pooled_ptr = mpi::ObjectPool<SubtourList>::pooled_unique_ptr;

    SubtourFinder(ObjectPools& object_pools)
        : cut_positions_pool(object_pools.cut_positions_pool),
          vector_of_tsp_size_pool(object_pools.vector_of_tsp_size_pool),
          subtour_list_pool(object_pools.subtour_list_pool) {}


    SubtourFinder(
        std::shared_ptr<mpi::ObjectPool<std::vector<std::tuple<size_t, size_t, size_t>>>> cut_positions_pool,
        std::shared_ptr<mpi::ObjectPool<std::vector<size_t>>> vector_of_tsp_size_pool,
        std::shared_ptr<mpi::ObjectPool<SubtourList>> subtour_list_pool)
        : cut_positions_pool(std::move(cut_positions_pool)),
          vector_of_tsp_size_pool(std::move(vector_of_tsp_size_pool)),
          subtour_list_pool(std::move(subtour_list_pool)) {}

    template <std::ranges::range ABCycles>
        requires std::convertible_to<std::ranges::range_value_t<ABCycles>, const ab_cycle_t&>
    subtour_list_pooled_ptr operator()(const std::vector<size_t>& pos,
                                        const ABCycles& applied_ab_cycles) {
        auto subtour_list_ptr = subtour_list_pool->acquire_unique();
        SubtourList& subtour_list = *subtour_list_ptr;
        subtour_list.clear();
                                                                    
        auto cut_positions_ptr = cut_positions_pool->acquire_unique();
        auto& cut_positions = *cut_positions_ptr;
        set_cut_positions(pos, cut_positions, applied_ab_cycles);
                                                                    
        auto pos_to_segment_id_ptr = vector_of_tsp_size_pool->acquire_unique();
        auto& pos_to_segment_id = *pos_to_segment_id_ptr;
        construct_segments(cut_positions, subtour_list, pos_to_segment_id);
                                                                    
        size_t sub_tour_count = set_sub_tour_ids(subtour_list, pos_to_segment_id);

        calc_sub_tour_sizes_and_merge_redundant_segments(subtour_list, sub_tour_count);
                                                                    
        return subtour_list_ptr;
    }
private:
    template <std::ranges::range ABCycles>
        requires std::convertible_to<std::ranges::range_value_t<ABCycles>, const eax::ab_cycle_t&>
    static void set_cut_positions(const std::vector<size_t>& pos,
                            std::vector<std::tuple<size_t, size_t, size_t>>& cut_positions,
                            const ABCycles& AB_cycles) {
        cut_positions.clear();
                            
        size_t city_count = pos.size();

        bool cut_between_first_and_last = false;
        auto cut = [&cut_positions, &cut_between_first_and_last, &pos, city_count](size_t b1, size_t ba, size_t ab, size_t b2) {
            auto pos_b1 = pos[b1];
            auto pos_ba = pos[ba];
            auto pos_ab = pos[ab];
            auto pos_b2 = pos[b2];
            
            if (pos_ba == 0 && pos_ab == city_count - 1) {
                cut_positions.emplace_back(pos_ba, pos_b1, pos_b2);
                cut_positions.emplace_back(city_count, pos_b1, pos_b2);
                cut_between_first_and_last = true;
            } else if (pos_ba == city_count - 1 && pos_ab == 0) {
                cut_positions.emplace_back(pos_ab, pos_b2, pos_b1);
                cut_positions.emplace_back(city_count, pos_b2, pos_b1);
                cut_between_first_and_last = true;
            } else if (pos_ba < pos_ab) {
                cut_positions.emplace_back(pos_ab, pos_b2, pos_b1);
            } else {
                cut_positions.emplace_back(pos_ba, pos_b1, pos_b2);
            }
        };
        for (const eax::ab_cycle_t& cycle : AB_cycles) {
            for (size_t i = 2; i < cycle.size() - 2; i += 2) {
                cut(cycle[i - 1], cycle[i], cycle[i + 1], cycle[i + 2]);
            }
            // i = 0
            {
                cut(cycle[cycle.size() - 1], cycle[0], cycle[1], cycle[2]);
            }
            // i = cycle.size() - 2
            {
                cut(cycle[cycle.size() - 3], cycle[cycle.size() - 2], cycle[cycle.size() - 1], cycle[0]);
            }
        }
        
        if (!cut_between_first_and_last) {
            cut_positions.emplace_back(0, city_count - 1, 0);
            cut_positions.emplace_back(city_count, city_count - 1, 0);
        }

        std::sort(cut_positions.begin(), cut_positions.end());
    }

    void construct_segments(std::vector<std::tuple<size_t, size_t, size_t>>& cut_positions,
                        eax::SubtourList& subtour_list,
                        std::vector<size_t>& pos_to_segment_id);
    size_t set_sub_tour_ids(eax::SubtourList& subtour_list,
                        std::vector<size_t>& pos_to_segment_id);
    void calc_sub_tour_sizes_and_merge_redundant_segments(eax::SubtourList& subtour_list,
                                                        size_t sub_tour_count);

    std::shared_ptr<mpi::ObjectPool<std::vector<std::tuple<size_t, size_t, size_t>>>> cut_positions_pool;
    std::shared_ptr<mpi::ObjectPool<std::vector<size_t>>> vector_of_tsp_size_pool;
    std::shared_ptr<mpi::ObjectPool<SubtourList>> subtour_list_pool;
};
}