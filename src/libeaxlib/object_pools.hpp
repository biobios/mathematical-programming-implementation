#pragma once

#include <memory>
#include <vector>

#include "object_pool.hpp"
#include "limited_range_integer_set.hpp"

#include "eaxdef.hpp"
#include "intermediate_individual.hpp"
#include "subtour_list.hpp"

namespace eax {
/**
 * @brief EAXの各オブジェクトが使用するオブジェクトプールを構築・保持するユーティリティ構造体
 */
struct ObjectPools {
    std::shared_ptr<mpi::ObjectPool<std::vector<size_t>>> vector_of_tsp_size_pool;
    std::shared_ptr<mpi::ObjectPool<std::vector<size_t>>> any_size_vector_pool;
    std::shared_ptr<mpi::ObjectPool<IntermediateIndividual>> intermediate_individual_pool;
    std::shared_ptr<mpi::ObjectPool<doubly_linked_list_t>> doubly_linked_list_pool;
    std::shared_ptr<mpi::ObjectPool<mpi::LimitedRangeIntegerSet>> LRIS_pool;
    std::shared_ptr<mpi::ObjectPool<std::vector<uint8_t>>> in_min_sub_tour_pool;
    std::shared_ptr<mpi::ObjectPool<std::vector<std::tuple<size_t, size_t, size_t>>>> cut_positions_pool;
    std::shared_ptr<mpi::ObjectPool<SubtourList>> subtour_list_pool;
    
    ObjectPools(size_t city_size)
        : vector_of_tsp_size_pool(std::make_shared<mpi::ObjectPool<std::vector<size_t>>>([city_size]() {
            return new std::vector<size_t>(city_size);
        })),
          any_size_vector_pool(std::make_shared<mpi::ObjectPool<std::vector<size_t>>>([]() {
            return new std::vector<size_t>();
        })),
          intermediate_individual_pool(std::make_shared<mpi::ObjectPool<IntermediateIndividual>>([city_size]() {
            return new IntermediateIndividual(city_size);
        })),
          doubly_linked_list_pool(std::make_shared<mpi::ObjectPool<doubly_linked_list_t>>([city_size]() {
            return new doubly_linked_list_t(city_size);
        })),
          LRIS_pool(std::make_shared<mpi::ObjectPool<mpi::LimitedRangeIntegerSet>>([city_size]() {
            return new mpi::LimitedRangeIntegerSet(city_size - 1, mpi::LimitedRangeIntegerSet::InitSet::Universal);
        })),
          in_min_sub_tour_pool(std::make_shared<mpi::ObjectPool<std::vector<uint8_t>>>([city_size]() {
            return new std::vector<uint8_t>(city_size, 0);
        })),
          cut_positions_pool(std::make_shared<mpi::ObjectPool<std::vector<std::tuple<size_t, size_t, size_t>>>>([]() {
            return new std::vector<std::tuple<size_t, size_t, size_t>>();
        })),
          subtour_list_pool(std::make_shared<mpi::ObjectPool<SubtourList>>([]() {
            return new SubtourList();
        })) {}
};
}