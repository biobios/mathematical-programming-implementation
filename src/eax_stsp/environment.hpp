#pragma once

#include <iostream>
#include "tsp_loader.hpp"
#include "limited_range_integer_set.hpp"
#include "object_pool.hpp"

namespace eax {
    class ObjectPools {
        const size_t city_count;
        mpi::ObjectPool<mpi::LimitedRangeIntegerSet> LRIS_pool;
        // TSPの都市数と同じサイズのvectorを生成するためのプール
        mpi::ObjectPool<std::vector<size_t>> vector_of_tsp_size_pool;
        mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
        mpi::ObjectPool<std::vector<size_t>> LRU_pool;
        mpi::ObjectPool<std::vector<std::array<size_t, 2>>> doubly_linked_list_pool;

        ObjectPools(size_t city_count) : city_count(city_count), 
            LRIS_pool([city_count]() {
                std::cout << "Creating LimitedRangeIntegerSet pool with max size: " << city_count - 1 << std::endl;
                return new mpi::LimitedRangeIntegerSet(city_count - 1, mpi::LimitedRangeIntegerSet::InitSet::Universal);
            }),
            vector_of_tsp_size_pool([city_count]() {
                std::cout << "Creating vector of TSP size pool with size: " << city_count << std::endl;
                return new std::vector<size_t>(city_count);
            }),
            any_size_vector_pool([city_count]() {
                std::cout << "Creating any size vector pool" << std::endl;
                auto vec = new std::vector<size_t>();
                vec->reserve(city_count / 2);
                return vec;
            }),
            doubly_linked_list_pool([city_count]() {
                std::cout << "Creating doubly linked list pool with size: " << city_count << std::endl;
                return new std::vector<std::array<size_t, 2>>(city_count);
            })
        {}
        
        ObjectPools(const ObjectPools& other) : ObjectPools(other.city_count) {}

    };

    struct Environment {
        tsp::TSP tsp;
        ObjectPools object_pools;
    };
}