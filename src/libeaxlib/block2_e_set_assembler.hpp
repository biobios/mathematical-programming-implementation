#pragma once

#include <cstddef>
#include <vector>
#include <random>

#include "object_pool.hpp"

#include "eaxdef.hpp"
#include "object_pools.hpp"

namespace eax {
/**
 * @brief ABサイクルを見つける関数オブジェクトのクラス
 */
class Block2ESetAssembler {
public:
    Block2ESetAssembler(size_t city_count,
                        mpi::pooled_unique_ptr<std::vector<size_t>>&& AB_cycle_size_ptr,
                        mpi::pooled_unique_ptr<std::vector<size_t>>&& c_vertex_count_ptr,
                        mpi::pooled_unique_ptr<std::vector<std::vector<size_t>>>&& shared_vertex_count_ptr,
                        mpi::ObjectPool<std::vector<size_t>>&& any_size_vector_pool)
        : city_count(city_count),
          cycle_count(AB_cycle_size_ptr->size()),
          AB_cycle_size_ptr(std::move(AB_cycle_size_ptr)),
          c_vertex_count_ptr(std::move(c_vertex_count_ptr)),
          shared_vertex_count_ptr(std::move(shared_vertex_count_ptr)),
          any_size_vector_pool(std::move(any_size_vector_pool)) {}
    
    /**
     * @brief 指定した中心ABサイクルに基づいてEセットを組み立てる
     * @param center_ab_cycle_index 中心ABサイクルのインデックス
     * @param rng 乱数生成器
     * @return Eセットに含まれるABサイクルのインデックスのベクター
     */
    mpi::pooled_unique_ptr<std::vector<size_t>> operator()(size_t center_ab_cycle_index,
                                                                        std::mt19937& rng);
private:
    size_t city_count;
    size_t cycle_count;
    mpi::pooled_unique_ptr<std::vector<size_t>> AB_cycle_size_ptr;
    mpi::pooled_unique_ptr<std::vector<size_t>> c_vertex_count_ptr;
    mpi::pooled_unique_ptr<std::vector<std::vector<size_t>>> shared_vertex_count_ptr;
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
};

/**
 * @brief Block2ESetAssemblerを構築するビルダークラス
 */
class Block2ESetAssemblerBuilder {
public:
    Block2ESetAssemblerBuilder(ObjectPools& object_pools)
        : vector_of_tsp_size_pool(object_pools.vector_of_tsp_size_pool.share()),
          any_size_vector_pool(object_pools.any_size_vector_pool.share()),
          shared_vertex_count_pool(object_pools.any_size_2d_vector_pool.share()) {}

    /**
     * @brief Block2ESetAssemblerを構築する
     * @param parent1 親個体1
     * @param parent2 親個体2
     * @param AB_cycles ABサイクルのポインタのベクター
     * @return Block2ESetAssemblerのインスタンス
     */
    Block2ESetAssembler create(const individual_readable auto& parent1, const individual_readable auto& parent2,
                               const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles) {
        using namespace std;
        
        size_t city_count = parent1.size();
        size_t cycle_count = AB_cycles.size();

        auto belongs_to_AB_cycle1_ptr = vector_of_tsp_size_pool.acquire_unique();
        auto belongs_to_AB_cycle2_ptr = vector_of_tsp_size_pool.acquire_unique();
        vector<size_t>& belongs_to_AB_cycle1 = *belongs_to_AB_cycle1_ptr;
        vector<size_t>& belongs_to_AB_cycle2 = *belongs_to_AB_cycle2_ptr;
        
        const size_t NULL_CYCLE = std::numeric_limits<size_t>::max();
        for (size_t i = 0; i < city_count; ++i) {
            belongs_to_AB_cycle1[i] = NULL_CYCLE;
            belongs_to_AB_cycle2[i] = NULL_CYCLE;
        }

        // 各ABサイクルのサイズを記録
        auto AB_cycle_size_ptr = any_size_vector_pool.acquire_unique();
        vector<size_t>& AB_cycle_size = *AB_cycle_size_ptr;
        AB_cycle_size.resize(cycle_count);
        for (size_t i = 0; i < cycle_count; ++i) {
            const auto& cycle = *AB_cycles[i];
            AB_cycle_size[i] = cycle.size();
        }
        
        // 各頂点が属するABサイクルを記録
        for (size_t i = 0; i < cycle_count; ++i) {
            const auto& cycle = *AB_cycles[i];
            for (auto city : cycle) {
                if (belongs_to_AB_cycle1[city] == NULL_CYCLE) {
                    belongs_to_AB_cycle1[city] = i;
                } else if (belongs_to_AB_cycle2[city] == NULL_CYCLE) {
                    belongs_to_AB_cycle2[city] = i;
                } else {
                    throw std::runtime_error("City belongs to more than 2 AB cycles");
                }
            }
        }
        
        // 無効ABサイクルを隣接する有効ABサイクルと統合する
        for (size_t i = 0; i < city_count; ++i) {
            if (belongs_to_AB_cycle1[i] != NULL_CYCLE && belongs_to_AB_cycle2[i] == NULL_CYCLE) {
                // AB_cyclesには、すべての有効ABサイクルが含まれているので、
                // 一方しか記録されていないならば、もう一方は無効ABサイクルである
                size_t AB_cycle_index = belongs_to_AB_cycle1[i];
                size_t v1 = i;
                
                // 有効ABサイクルに属している方の頂点
                size_t v_belonging_to_AB_cycle = 0;
                // 無効ABサイクルは親間で一致している辺で構成されるので
                // そうではない方を見ればよい
                if (parent1[v1][0] != parent2[v1][0] && parent1[v1][0] != parent2[v1][1]) {
                    v_belonging_to_AB_cycle = parent1[v1][0];
                } else if (parent1[v1][1] != parent2[v1][0] && parent1[v1][1] != parent2[v1][1]) {
                    v_belonging_to_AB_cycle = parent1[v1][1];
                } else {
                    throw std::runtime_error("Invalid AB cycle");
                }
                
                // 無効ABサイクルをたどって、その頂点を有効ABサイクルAB_cycle_indexに追加する
                while (true) {
                    belongs_to_AB_cycle2[v1] = AB_cycle_index;
                    size_t next_v1 = parent1[v1][0];
                    if (next_v1 == v_belonging_to_AB_cycle) {
                        next_v1 = parent1[v1][1];
                    }
                    
                    if (belongs_to_AB_cycle1[next_v1] == NULL_CYCLE) {
                        belongs_to_AB_cycle1[next_v1] = AB_cycle_index;
                    } else if (belongs_to_AB_cycle2[next_v1] == NULL_CYCLE) {
                        belongs_to_AB_cycle2[next_v1] = AB_cycle_index;
                        break; // 無効ABサイクルの系列の終端に到達した
                    } else {
                        throw std::runtime_error("Invalid AB cycle");
                    }
                    
                    v_belonging_to_AB_cycle = v1;
                    v1 = next_v1;
                }
            }
        }
        
        // 初期化
        auto c_vertex_count_ptr = vector_of_tsp_size_pool.acquire_unique();
        auto& c_vertex_count = *c_vertex_count_ptr;
        c_vertex_count.assign(cycle_count, 0);

        auto shared_vertex_count_ptr = shared_vertex_count_pool.acquire_unique();
        auto& shared_vertex_count = *shared_vertex_count_ptr;
        shared_vertex_count.resize(cycle_count);
        for (size_t i = 0; i < cycle_count; ++i) {
            shared_vertex_count[i].assign(cycle_count, 0);
        }
        
        // C頂点の数と共有頂点の数をカウント
        for (size_t i = 0; i < city_count; ++i) {
            if (belongs_to_AB_cycle1[i] != belongs_to_AB_cycle2[i]) {
                ++c_vertex_count[belongs_to_AB_cycle1[i]];
                ++c_vertex_count[belongs_to_AB_cycle2[i]];
                if (belongs_to_AB_cycle1[i] != NULL_CYCLE && belongs_to_AB_cycle2[i] != NULL_CYCLE) {
                    ++shared_vertex_count[belongs_to_AB_cycle1[i]][belongs_to_AB_cycle2[i]];
                    ++shared_vertex_count[belongs_to_AB_cycle2[i]][belongs_to_AB_cycle1[i]];
                }
            }
        }
        
        return Block2ESetAssembler(city_count,
                                  std::move(AB_cycle_size_ptr),
                                  std::move(c_vertex_count_ptr),
                                  std::move(shared_vertex_count_ptr),
                                  any_size_vector_pool.share());
    }
private:
    mpi::ObjectPool<std::vector<size_t>> vector_of_tsp_size_pool;
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
    mpi::ObjectPool<std::vector<std::vector<size_t>>> shared_vertex_count_pool;
};
}