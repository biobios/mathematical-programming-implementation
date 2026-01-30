#pragma once

#include <random>

#include "basic_individual.hpp"
#include "crossover_delta.hpp"

namespace eax {

/**
 * @brief タブーエッジを管理する個体クラス
 */
class SampleTabuIndividual : public WritableWithBasicIndividual<SampleTabuIndividual> {
public:
    SampleTabuIndividual(const std::vector<size_t>& path, const adjacency_matrix_t& adjacency_matrix, size_t tabu_range = 5)
        : WritableWithBasicIndividual<SampleTabuIndividual>(path, adjacency_matrix),
          pending_delta(*this),
          tabu_range(tabu_range),
          tabu_edges(tabu_range) {}
    
    SampleTabuIndividual& operator=(const CrossoverDelta& delta) {
        pending_delta = delta;
        return *this;
    }

    SampleTabuIndividual& operator=(CrossoverDelta&& delta) {
        pending_delta = std::move(delta);
        return *this;
    }

    /**
     * @brief タブーリストを更新する
     * @param delta 適用されたデルタ
     * @param rng 乱数生成器
     */
    void update_tabu(const CrossoverDelta& delta, std::uniform_random_bit_generator auto& rng) {
        // 使用済みのタブーリストをクリアし、次のタブーリストに移動
        tabu_edges[current_tabu_index].clear();
        current_tabu_index = (current_tabu_index + 1) % tabu_range;
        
        // 変更をもとにタブーリストを更新
        // modificationには、同一の辺が2回ずつ含まれているため、
        // まったく選ばれない確率が0.5となるように確率を設定
        std::bernoulli_distribution tabu_decision(1.0 - std::sqrt(0.5));

        for (auto& tabu_edge_list : tabu_edges) {
            for (const auto& modification : delta.get_modifications()) {
                auto [v1, v2] = modification.edge1;
                auto new_v2 = modification.new_v2;

                if (tabu_decision(rng)) {
                    tabu_edge_list.emplace_back(v1, v2);
                }

                if (tabu_decision(rng)) {
                    tabu_edge_list.emplace_back(v1, new_v2);
                }
            }
        }
    }
    
    /**
     * @brief 現在のタブーエッジを取得する
     */
    std::vector<std::pair<size_t, size_t>> const& get_tabu_edges() const {
        return tabu_edges[current_tabu_index];
    }

private:
    CrossoverDelta pending_delta;
    size_t tabu_range;
    std::vector<std::vector<std::pair<size_t, size_t>>> tabu_edges;
    size_t current_tabu_index = 0;
};

static_assert(individual_readable<SampleTabuIndividual>);
static_assert(individual_writable<SampleTabuIndividual>);

}  // namespace eax
