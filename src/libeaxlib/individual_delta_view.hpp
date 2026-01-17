#pragma once

#include "eaxdef.hpp"
#include "crossover_delta.hpp"

namespace eax {

/**
 * @brief 個体クラスとCrossoverDeltaの組み合わせを表すビュー
 *      個体クラスとCrossoverDeltaの橋渡しをする
 * @tparam T 個体の型
 */
template <individual_concept T>
struct IndividualDeltaView {
    const T& parent;
    const CrossoverDelta& delta;

    IndividualDeltaView(const T& parent)
        : parent(parent), delta(empty_delta) {}

    IndividualDeltaView(const T& parent, const CrossoverDelta& delta)
        : parent(parent), delta(delta) {
        if (!delta.is_base_individual(parent)) {
            throw std::invalid_argument("IndividualDeltaView: The provided parent individual does not match the base individual of the delta.");
        }
    }

    void apply_to(T& individual) const {
        if (!delta.is_base_individual(individual)) {
            individual = parent; // ベース個体でなければコピーする
        }

        delta.apply_to(individual);
    }

    static inline const CrossoverDelta empty_delta{};
};
}