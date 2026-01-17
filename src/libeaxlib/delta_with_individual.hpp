#pragma once

#include "eaxdef.hpp"
#include "crossover_delta.hpp"

namespace eax {
template <individual_concept T>
class DeltaWithIndividual {
public:
    /**
     * @brief コンストラクタ
     * @param individual ベースの個体
     * @param delta 変更内容
     * @throws std::invalid_argument ベースの個体がdeltaのベース個体と一致しない場合
     */
    DeltaWithIndividual(const T& individual, const CrossoverDelta& delta)
        : individual(individual), delta(delta) {
        if (!delta.is_base_individual(individual)) {
            throw std::invalid_argument("DeltaWithIndividual: The provided individual does not match the base individual of the delta.");
        }
    }

    /**
     * @brief コンストラクタ
     * @param individual ベースの個体
     * @param delta 変更内容
     * @throws std::invalid_argument ベースの個体がdeltaのベース個体と一致しない場合
     */
    DeltaWithIndividual(const T& individual, const CrossoverDelta&& delta)
        : individual(individual), delta(std::move(delta)) {
        if (!this->delta.is_base_individual(individual)) {
            throw std::invalid_argument("DeltaWithIndividual: The provided individual does not match the base individual of the delta.");
        }
    }

    /**
     * @brief コンストラクタ (個体とDeltaWithIndividualを統一的に扱うためのもの)
     * @param individual ベースの個体
     */
    DeltaWithIndividual(const T& individual)
        : individual(individual), delta() {}
    

    void apply_to(T& target_individual) const {
        if (!delta.is_base_individual(target_individual)) {
            target_individual = individual; // ベース個体でなければコピーする
        }

        delta.apply_to(target_individual);
    }

    const T& individual;
    const CrossoverDelta delta;
};
}