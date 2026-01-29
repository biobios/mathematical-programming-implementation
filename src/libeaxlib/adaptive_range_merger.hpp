#pragma once

#include <ranges>
#include <cassert>

#include "object_pool.hpp"

#include "object_pools.hpp"
#include "tsp_loader.hpp"
#include "eaxdef.hpp"
#include "edge_counter.hpp"
#include "intermediate_individual.hpp"
#include "subtour_finder.hpp"
#include "subtour_merger.hpp"

namespace eax {
class AdaptiveRangeMerger {
public:
    AdaptiveRangeMerger(ObjectPools& object_pools)
        : any_size_vector_pool(object_pools.any_size_vector_pool.share()),
          in_min_sub_tour_pool(object_pools.in_min_sub_tour_pool.share()),
          subtour_finder(object_pools),
          default_merger(object_pools) {}

    template <std::ranges::range ABCycles>
        requires std::convertible_to<std::ranges::range_value_t<ABCycles>, const ab_cycle_t&>
    void operator()(IntermediateIndividual& working_individual,
                    const tsp::TSP& tsp,
                    const ABCycles& applied_ab_cycles,
                    const EdgeCounter& edge_counter,
                    const std::size_t average_neighbor_range) {
        
        if (average_neighbor_range == 0) {
            throw std::invalid_argument("average_neighbor_range must be greater than 0");
        }

        auto& adjacency_matrix = tsp.adjacency_matrix;
        auto& NN_list = tsp.NN_list;
        auto& path = working_individual.get_path();
        auto& pos = working_individual.get_pos();
                                    
        auto subtour_list_ptr = subtour_finder(pos, applied_ab_cycles);
        SubtourList& subtour_list = *subtour_list_ptr;

        using namespace std;
        using distance_type = std::remove_cvref_t<decltype(adjacency_matrix)>::value_type::value_type;
        using edge = pair<size_t, size_t>;

        auto elem_of_min_sub_tour_ptr = any_size_vector_pool.acquire_unique();
        auto in_min_sub_tour_ptr = in_min_sub_tour_pool.acquire_unique();
        vector<size_t>& elem_of_min_sub_tour = *elem_of_min_sub_tour_ptr;
        vector<uint8_t>& in_min_sub_tour = *in_min_sub_tour_ptr;

        // 頂点に接続されているユニークな辺の平均数を計算
        const double average_unique_edge_count = static_cast<double>(edge_counter.get_unique_edge_count() * 2) / tsp.city_count;
        
        if (average_unique_edge_count <= 2.0) {
            // average_unique_edge_count が 2.0 以下の場合、デフォルトのマージャーに委譲する
            default_merger(working_individual, tsp, applied_ab_cycles);
            return;
        }

        while (subtour_list.sub_tour_count() > 1) {
            auto [min_sub_tour_id, min_sub_tour_size] = subtour_list.find_min_size_sub_tour();
            size_t start_city = path[subtour_list.get_city_pos_of_sub_tour(min_sub_tour_id)];

            elem_of_min_sub_tour.clear();
            elem_of_min_sub_tour.reserve(min_sub_tour_size + 2);
            size_t prev_city = start_city;
            size_t current_city = start_city;
            do {
                elem_of_min_sub_tour.push_back(current_city);
                in_min_sub_tour[current_city] = true;
                size_t next_city = working_individual[current_city][0];
                if (next_city == prev_city) {
                    next_city = working_individual[current_city][1];
                }
                prev_city = current_city;
                current_city = next_city;
            } while (current_city != start_city);
            
            elem_of_min_sub_tour.push_back(elem_of_min_sub_tour[0]);
            elem_of_min_sub_tour.push_back(elem_of_min_sub_tour[1]);
            
            size_t start_coefficient = 0;
            size_t end_coefficient = 1;
            edge e1 = {0, 0};
            edge e2 = {0, 0};
            distance_type min_cost = std::numeric_limits<distance_type>::max();

            while (e1.first == 0 && e2.first == 0) {
                for (size_t i = 1; i <= min_sub_tour_size; ++i) {
                    size_t current_city = elem_of_min_sub_tour[i];

                    size_t unique_connections = edge_counter.get_connected_vertices(current_city).size();
                    double range_multiplier = 1 + (average_neighbor_range - 1) * ((unique_connections - 2) / (average_unique_edge_count - 2.0));
                    
                    // double to size_t の変換によるオーバーフローを防ぐため、上限を city_count に設定
                    double max_multiplier = tsp.city_count;
                    if (range_multiplier > max_multiplier) {
                        range_multiplier = max_multiplier;
                    }

                    size_t start = start_coefficient * range_multiplier;
                    size_t end = end_coefficient * range_multiplier;

                    size_t limit = std::min(end, NN_list[current_city].size());

                    for (size_t j = start; j < limit; ++j) {
                        size_t neighbor_city = NN_list[current_city][j].second;
                        if (in_min_sub_tour[neighbor_city])
                            continue;

                        for (size_t k = 0; k < 2; ++k) {
                            size_t connected_to_current_city = elem_of_min_sub_tour[i - 1 + 2 * k];
                            for (size_t l = 0; l < 2; ++l) {
                                size_t connected_to_neighbor_city = working_individual[neighbor_city][l];

                                distance_type cost = - adjacency_matrix[current_city][connected_to_current_city] - adjacency_matrix[neighbor_city][connected_to_neighbor_city]
                                                    + adjacency_matrix[current_city][neighbor_city] + adjacency_matrix[connected_to_current_city][connected_to_neighbor_city];
                                
                                if (cost < min_cost) {
                                    min_cost = cost;
                                    e1 = {current_city, connected_to_current_city};
                                    e2 = {neighbor_city, connected_to_neighbor_city};
                                }
                                
                                cost = - adjacency_matrix[current_city][connected_to_current_city] - adjacency_matrix[neighbor_city][connected_to_neighbor_city]
                                    + adjacency_matrix[current_city][connected_to_neighbor_city] + adjacency_matrix[connected_to_current_city][neighbor_city];

                                if (cost < min_cost) {
                                    min_cost = cost;
                                    e1 = {current_city, connected_to_current_city};
                                    e2 = {connected_to_neighbor_city, neighbor_city};
                                }
                            }
                        }
                    }
                }
                
                start_coefficient = end_coefficient;
                end_coefficient *= 2;
            }
            
            
            working_individual.swap_edges(e1, e2);
            size_t connected_to_min_sub_tour = subtour_list.find_sub_tour_containing(pos[e2.first]);
            subtour_list.merge_sub_tour(min_sub_tour_id, connected_to_min_sub_tour);
            
            for (size_t city : elem_of_min_sub_tour) {
                in_min_sub_tour[city] = false;
            }
        }
    }
private:
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
    mpi::ObjectPool<std::vector<uint8_t>> in_min_sub_tour_pool;
    SubtourFinder subtour_finder;
    /**
     * @brief average_unique_edge_count <= 2.0 のときに、委譲するマージャー
     */
    SubtourMerger default_merger;
};
}