#pragma once

#include "eaxdef.hpp"
#include "crossover_delta.hpp"

namespace eax {

template <typename Individual>
class DeltaWithIndividual {
public:
    /**
     * @brief コンストラクタ
     * @param individual ベースの個体
     * @param delta 変更内容
     * @throws std::invalid_argument ベースの個体がdeltaのベース個体と一致しない場合
     */
    DeltaWithIndividual(const Individual& individual, const CrossoverDelta& delta)
        requires individual_readable<Individual>
        : individual_ptr(&individual), delta(delta) {
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
    DeltaWithIndividual(const Individual& individual, CrossoverDelta&& delta)
        requires individual_readable<Individual>
        : individual_ptr(&individual), delta(std::move(delta)) {
        if (!this->delta.is_base_individual(individual)) {
            throw std::invalid_argument("DeltaWithIndividual: The provided individual does not match the base individual of the delta.");
        }
    }

    /**
     * @brief コンストラクタ (個体とDeltaWithIndividualを統一的に扱うためのもの)
     * @param individual ベースの個体
     */
    DeltaWithIndividual(const Individual& individual)
        : individual_ptr(&individual), delta(individual) {}

    void apply_to(Individual& target_individual) const 
        requires individual_writable<Individual> && std::assignable_from<Individual&, const Individual&>
    {
        if (!delta.is_base_individual(target_individual)) {
            target_individual = *individual_ptr; // ベース個体でなければコピーする
        }

        delta.apply_to(target_individual);
    }

    const Individual& get_individual() const {
        return *individual_ptr;
    }

    const CrossoverDelta& get_delta() const {
        return delta;
    }

    /**
     * @brief 右辺値のthisについて、このオブジェクトを個体とデルタのペアに分解する
     * @return 個体とデルタのペア
     */
    operator std::pair<const Individual*, CrossoverDelta>() && {
        return {individual_ptr, std::move(delta)};
    }

private:
    const Individual* individual_ptr;
    CrossoverDelta delta;
};
}