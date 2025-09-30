#include "individual.hpp"

#include <random>

#include "context.hpp"

namespace eax {
    Individual::Individual(const std::vector<size_t>& path, const std::vector<std::vector<int64_t>>& adjacency_matrix, size_t tabu_range) {
        this->tabu_range = tabu_range;
        tabu_edges.resize(tabu_range);

        doubly_linked_list.resize(path.size());
        for (size_t i = 1; i < path.size() - 1; ++i) {
            size_t prev_index = i - 1;
            size_t next_index = i + 1;
            doubly_linked_list[path[i]][0] = path[prev_index];
            doubly_linked_list[path[i]][1] = path[next_index];
            distance += adjacency_matrix[path[i]][path[next_index]];
        }
        
        doubly_linked_list[path[0]][0] = path.back();
        doubly_linked_list[path[0]][1] = path[1];
        distance += adjacency_matrix[path[0]][path[1]];
        doubly_linked_list[path.back()][0] = path[path.size() - 2];
        doubly_linked_list[path.back()][1] = path[0];
        distance += adjacency_matrix[path.back()][path[0]];
    }
    
    size_t Individual::size() const {
        return doubly_linked_list.size();
    }

    std::vector<size_t> Individual::to_path() const
    {
        std::vector<size_t> path;
        size_t current_city = 0;
        size_t prev_city = 0;
        for (size_t i = 0; i < doubly_linked_list.size(); ++i) {
            path.push_back(current_city);
            size_t next_city = doubly_linked_list[current_city][0];
            if (next_city == prev_city) {
                next_city = doubly_linked_list[current_city][1];
            }
            prev_city = current_city;
            current_city = next_city;
        }
        return path;
    }
    
    void Individual::serialize(std::ostream& os) const {
        os << distance << " " << doubly_linked_list.size() << " ";
        for (size_t i = 0; i < doubly_linked_list.size(); ++i) {
            os << doubly_linked_list[i][0] << " " << doubly_linked_list[i][1] << " ";
        }
        os << tabu_range << " " << current_tabu_index;
        for (const auto& tabu_list : tabu_edges) {
            os << tabu_list.size() << " ";
            for (const auto& [v1, v2] : tabu_list) {
                os << v1 << " " << v2 << " ";
            }
        }
    }
    
    Individual Individual::deserialize(std::istream& is) {
        Individual individual;
        is >> individual.distance;
        size_t size;
        is >> size;
        individual.doubly_linked_list.resize(size);
        for (size_t i = 0; i < size; ++i) {
            is >> individual.doubly_linked_list[i][0] >> individual.doubly_linked_list[i][1];
        }
        is >> individual.tabu_range >> individual.current_tabu_index;
        individual.tabu_edges.resize(individual.tabu_range);
        for (size_t i = 0; i < individual.tabu_range; ++i) {
            size_t tabu_list_size;
            is >> tabu_list_size;
            individual.tabu_edges[i].resize(tabu_list_size);
            for (size_t j = 0; j < tabu_list_size; ++j) {
                is >> individual.tabu_edges[i][j].first >> individual.tabu_edges[i][j].second;
            }
        }
        return individual;
    }

    CrossoverDelta Individual::update(eax::Context& context) {
        CrossoverDelta child = std::move(prev_diff);
        prev_diff = CrossoverDelta();
        // グラフの更新
        child.apply_to(*this);
        // 距離の更新
        distance += child.get_delta_distance(context.env.tsp.adjacency_matrix);
        // タブーリストの更新
        tabu_edges[current_tabu_index].clear();
        current_tabu_index = (current_tabu_index + 1) % tabu_range;
        auto& modifications = child.get_modifications();
        std::bernoulli_distribution tabu_decision(0.5);
        for (size_t i = 0; i < tabu_range; ++i) {
            for (size_t j = 0; j < modifications.size(); j += 2) {
                auto [v1, v2] = modifications[j].edge1;
                auto new_v2 = modifications[j].new_v2;
                if (tabu_decision(context.random_gen)) {
                    tabu_edges[i].emplace_back(v1, v2);
                }
                if (tabu_decision(context.random_gen)) {
                    tabu_edges[i].emplace_back(v1, new_v2);
                }
            }
        }
        return child;
    }
}