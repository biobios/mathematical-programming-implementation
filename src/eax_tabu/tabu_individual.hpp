#pragma once

#include <random>

#include "basic_individual.hpp"
#include "crossover_delta.hpp"

class TabuIndividual : public eax::ReadableWithBasicIndividual<TabuIndividual> {
public:
    TabuIndividual(const std::vector<size_t>& path, const eax::adjacency_matrix_t& adjacency_matrix, size_t tabu_range = 5)
        : ReadableWithBasicIndividual<TabuIndividual>(path, adjacency_matrix),
            pending_delta(*this),
            tabu_range(tabu_range),
            tabu_edges(tabu_range) {}
    
    TabuIndividual& operator=(const eax::CrossoverDelta& delta);
    TabuIndividual& operator=(eax::CrossoverDelta&& delta);

    /**
     * @brief 保留された変更を適用し、タブーリストを更新する
     * @return 適用された変更内容
     */
    eax::CrossoverDelta update_graph_and_tabu(std::uniform_random_bit_generator auto& random_gen) {
        pending_delta.apply_to(this->individual);
        
        // 使用済みのタブーリストをクリアし、次のタブーリストに移動
        tabu_edges[current_tabu_index].clear();
        current_tabu_index = (current_tabu_index + 1) % tabu_range;
        
        // 変更をもとにタブーリストを更新
        std::bernoulli_distribution tabu_decision(0.5);

        for (auto& tabu_edge_list : tabu_edges) {
            for (const auto& modification : pending_delta.get_modifications()) {
                auto [v1, v2] = modification.edge1;
                auto new_v2 = modification.new_v2;

                if (tabu_decision(random_gen)) {
                    tabu_edge_list.emplace_back(v1, v2);
                }

                if (tabu_decision(random_gen)) {
                    tabu_edge_list.emplace_back(v1, new_v2);
                }
            }
        }

        // 適用された変更内容を返す
        eax::CrossoverDelta applied_delta = std::move(pending_delta);
        pending_delta = eax::CrossoverDelta(*this);

        return applied_delta;
    }
    
    /**
     * @brief 現在のタブーエッジを取得する
     * @return タブーエッジのリスト
     */
    std::vector<std::pair<size_t, size_t>> const& get_tabu_edges() const;

private:
    eax::CrossoverDelta pending_delta;
    size_t tabu_range;
    std::vector<std::vector<std::pair<size_t, size_t>>> tabu_edges;
    size_t current_tabu_index = 0;
};