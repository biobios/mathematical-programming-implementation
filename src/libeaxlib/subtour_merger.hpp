#pragma once

#include <memory>
#include <vector>

#include "object_pool.hpp"

#include "object_pools.hpp"
#include "tsp_loader.hpp"
#include "intermediate_individual.hpp"
#include "subtour_finder.hpp"

namespace eax {
class SubtourMerger {
public:
    SubtourMerger(ObjectPools& object_pools)
        : any_size_vector_pool(object_pools.any_size_vector_pool),
          in_min_sub_tour_pool(object_pools.in_min_sub_tour_pool),
          subtour_finder(object_pools) {}

    SubtourMerger(
        std::shared_ptr<mpi::ObjectPool<std::vector<size_t>>> any_size_vector_pool,
        std::shared_ptr<mpi::ObjectPool<std::vector<uint8_t>>> in_min_sub_tour_pool,
        SubtourFinder subtour_finder)
        : any_size_vector_pool(std::move(any_size_vector_pool)),
          in_min_sub_tour_pool(std::move(in_min_sub_tour_pool)),
          subtour_finder(std::move(subtour_finder)) {}

    template <std::ranges::range ABCycles>
        requires std::convertible_to<std::ranges::range_value_t<ABCycles>, const ab_cycle_t&>
    void operator()(IntermediateIndividual& working_individual,
                                    const tsp::TSP& tsp,
                                    const ABCycles& applied_ab_cycles) {

        auto& adjacency_matrix = tsp.adjacency_matrix;
        auto& NN_list = tsp.NN_list;
        auto& path = working_individual.get_path();
        auto& pos = working_individual.get_pos();
                                    
        auto subtour_list_ptr = subtour_finder(pos, applied_ab_cycles);
        SubtourList& subtour_list = *subtour_list_ptr;

        using namespace std;
        using distance_type = std::remove_cvref_t<decltype(adjacency_matrix)>::value_type::value_type;
        using edge = pair<size_t, size_t>;

        auto elem_of_min_sub_tour_ptr = any_size_vector_pool->acquire_unique();
        auto in_min_sub_tour_ptr = in_min_sub_tour_pool->acquire_unique();
        vector<size_t>& elem_of_min_sub_tour = *elem_of_min_sub_tour_ptr;
        vector<uint8_t>& in_min_sub_tour = *in_min_sub_tour_ptr;

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
            
            size_t search_range = 10;
            size_t start = 0;
            edge e1 = {0, 0};
            edge e2 = {0, 0};
            distance_type min_cost = std::numeric_limits<distance_type>::max();
            while (e1.first == 0 && e2.first == 0) {
                for (size_t i = 1; i <= min_sub_tour_size; ++i) {
                    size_t current_city = elem_of_min_sub_tour[i];
                    size_t limit = std::min(start + search_range, NN_list[current_city].size());
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
                
                start += search_range;
                search_range *= 2;
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
    std::shared_ptr<mpi::ObjectPool<std::vector<size_t>>> any_size_vector_pool;
    std::shared_ptr<mpi::ObjectPool<std::vector<uint8_t>>> in_min_sub_tour_pool;
    SubtourFinder subtour_finder;
};
}