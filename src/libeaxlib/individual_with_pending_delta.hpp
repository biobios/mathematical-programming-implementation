#pragma once

#include "eaxdef.hpp"
#include "basic_individual.hpp"
#include "crossover_delta.hpp"

namespace eax {
class IndividualWithPendingDelta : public ReadableWithBasicIndividual<IndividualWithPendingDelta> {
public:
    using DeltaType = CrossoverDelta;

    IndividualWithPendingDelta(const std::vector<size_t>& path, const adjacency_matrix_t& adjacency_matrix)
        : ReadableWithBasicIndividual<IndividualWithPendingDelta>(path, adjacency_matrix),
          pending_delta(*this) {}

    /**
     * @brief *thisをベースとするCrossoverDeltaを渡し、保留する
     * @return *this
     */
    IndividualWithPendingDelta& operator=(const CrossoverDelta& delta);

    /**
     * @brief *thisをベースとするCrossoverDeltaを渡し、保留する
     * @return *this
     */
    IndividualWithPendingDelta& operator=(CrossoverDelta&& delta);
    
    /**
     * @brief 保留している変更を個体に適用し、CrossoverDeltaを返す
     * @return 適用したCrossoverDelta
     */
    CrossoverDelta apply_pending_delta();

private:
    DeltaType pending_delta;
};

static_assert(individual_readable<IndividualWithPendingDelta>);
static_assert(!individual_writable<IndividualWithPendingDelta>);
}