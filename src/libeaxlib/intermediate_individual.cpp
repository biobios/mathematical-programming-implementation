#include "intermediate_individual.hpp"

#include <utility>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace eax {
IntermediateIndividual::IntermediateIndividual(size_t size)
    : individual_being_edited(size),
      modifications(),
      path(size),
      pos(size) {
    
    // 適当に初期化
    for (size_t i = 0; i < size; ++i) {
        path[i] = i;
        pos[i] = i;
    }
    
    for (size_t i = 1; i < size - 1; ++i) {
        individual_being_edited[i] = {i - 1, i + 1};
    }
    individual_being_edited[0] = {size - 1, 1};
    individual_being_edited[size - 1] = {size - 2, 0};
}

CrossoverDelta IntermediateIndividual::get_delta_and_revert() {
    revert();
    CrossoverDelta delta(std::move(modifications));
    reset();
    return delta;
}

void IntermediateIndividual::discard() {
    revert();
    reset();
}

const std::array<size_t, 2>& IntermediateIndividual::operator[](size_t index) const {
    return individual_being_edited[index];
}

void IntermediateIndividual::change_connection(size_t v1, size_t v2, size_t new_v2) {
    modifications.emplace_back(std::pair{v1, v2}, new_v2);

    if (individual_being_edited[v1][0] == v2) {
        individual_being_edited[v1][0] = new_v2;
    } else {
        individual_being_edited[v1][1] = new_v2;
    }
}

void IntermediateIndividual::swap_edges(std::pair<size_t, size_t> edge1, std::pair<size_t, size_t> edge2) {
    auto [v1, v2] = edge1;
    auto [u1, u2] = edge2;
    // v1 -> v2 => v1 -> u1
    change_connection(v1, v2, u1);
    // v2 -> v1 => v2 -> u2
    change_connection(v2, v1, u2);
    // u2 -> u1 => u2 -> v2
    change_connection(u2, u1, v2);
    // u1 -> u2 => u1 -> v1
    change_connection(u1, u2, v1);
}

const std::vector<size_t>& IntermediateIndividual::get_path() const {
    return path;
}

const std::vector<size_t>& IntermediateIndividual::get_pos() const {
    return pos;
}

int64_t IntermediateIndividual::calc_delta_distance(const std::vector<std::vector<int64_t>>& adjacency_matrix) const {
    int64_t delta_distance = 0;
    for (auto& modification : modifications) {
        auto [v1, v2] = modification.edge1;
        size_t new_v2 = modification.new_v2;
        delta_distance -= adjacency_matrix[v1][v2];
        delta_distance += adjacency_matrix[v1][new_v2];
    }
    
    delta_distance /= 2;
    return delta_distance;
}

void IntermediateIndividual::revert() {
    for (auto it = modifications.crbegin(); it != modifications.crend(); ++it) {
        undo(*it);
    }
}

/**
 * @brief individual_being_edited, path, posを除くすべてのメンバ変数を初期化する。
 * @note この関数は、revert()が呼び出された後、change_connection()やswap_edges()が呼び出される前に呼び出されることを想定している。
 */
void IntermediateIndividual::reset() {
    modifications.clear();
}

void IntermediateIndividual::undo(const CrossoverDelta::Modification& modification) {
    auto [v1, v2] = modification.edge1;
    size_t new_v2 = modification.new_v2;
    if (individual_being_edited[v1][0] == new_v2) {
        individual_being_edited[v1][0] = v2;
    } else {
        individual_being_edited[v1][1] = v2;
    }
}

}