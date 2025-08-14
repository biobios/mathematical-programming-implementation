#pragma once

#include <vector>
#include <functional>
#include <chrono>

#include "tsp_loader.hpp"
#include "object_pool.hpp"
#include "limited_range_integer_set.hpp"
#include "eax.hpp"

namespace eax {
    enum class EAXType {
        Rand,
        N_AB,
        Block2,
    };

    enum class SelectionType {
        Greedy,
        LDL,
        Ent,
    };
    
    struct ObjectPools {
        const size_t city_count;
        mpi::ObjectPool<mpi::LimitedRangeIntegerSet> LRIS_pool;
        mpi::ObjectPool<eax::IntermediateIndividual> intermediate_individual_pool;
        // TSPの都市数と同じサイズのvectorを生成するためのプール
        mpi::ObjectPool<std::vector<size_t>> vector_of_tsp_size_pool;
        mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
        mpi::ObjectPool<std::vector<size_t>> LRU_pool;
        // mpi::ObjectPool<std::vector<bool>, std::function<std::vector<bool>*()>> in_min_sub_tour_pool;
        mpi::ObjectPool<std::vector<uint8_t>> in_min_sub_tour_pool;
        mpi::ObjectPool<std::vector<std::tuple<size_t, size_t, size_t>>> cut_positions_pool;
        mpi::ObjectPool<std::vector<std::array<size_t, 2>>> doubly_linked_list_pool;

        ObjectPools(size_t city_count) : city_count(city_count), 
            LRIS_pool([city_count]() {
                std::cout << "Creating LimitedRangeIntegerSet pool with max size: " << city_count - 1 << std::endl;
                return new mpi::LimitedRangeIntegerSet(city_count - 1, mpi::LimitedRangeIntegerSet::InitSet::Universal);
            }),
            intermediate_individual_pool([]() {
                std::cout << "Creating IntermediateIndividual pool" << std::endl;
                return new eax::IntermediateIndividual();
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
            LRU_pool([city_count]() {
                std::cout << "Creating LRU pool with size: " << city_count << std::endl;
                return new std::vector<size_t>(city_count, 0);
            }),
            in_min_sub_tour_pool([city_count]() {
                std::cout << "Creating in_min_sub_tour pool with size: " << city_count << std::endl;
                return new std::vector<uint8_t>(city_count, 0);
            }),
            cut_positions_pool([]() {
                std::cout << "Creating cut positions pool" << std::endl;
                return new std::vector<std::tuple<size_t, size_t, size_t>>();
            }),
            doubly_linked_list_pool([city_count]() {
                std::cout << "Creating doubly linked list pool with size: " << city_count << std::endl;
                return new std::vector<std::array<size_t, 2>>(city_count);
            })
        {}
        
        ObjectPools(const ObjectPools& other) : ObjectPools(other.city_count) {}
        
    private:
    };

    struct Environment {
        tsp::TSP tsp;
        size_t N_parameter;
        size_t population_size;
        size_t num_children;
        EAXType eax_type;
        SelectionType selection_type;
        std::vector<std::vector<size_t>> pop_edge_counts; // 各エッジの個数
        ObjectPools object_pools;

        Environment(size_t city_count) : object_pools(city_count) {}

        void set_initial_edge_counts(const std::vector<Individual>& init_pop) {
            pop_edge_counts.resize(tsp.city_count, std::vector<size_t>(tsp.city_count, 0));
            
            for (const auto& individual : init_pop) {
                for (size_t i = 0; i < individual.size(); ++i) {
                    size_t v1 = individual[i][0];
                    size_t v2 = individual[i][1];
                    pop_edge_counts[i][v1] += 1;
                    pop_edge_counts[i][v2] += 1;
                }
            }
        };
    };
}