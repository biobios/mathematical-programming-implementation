#include "individual_with_pending_delta.hpp"

namespace eax {
    IndividualWithPendingDelta &IndividualWithPendingDelta::operator=(const CrossoverDelta &delta_view)
    {
        pending_delta = delta_view;
        return *this;
    }

    IndividualWithPendingDelta &IndividualWithPendingDelta::operator=(CrossoverDelta &&delta_view)
    {
        pending_delta = std::move(delta_view);
        return *this;
    }

    CrossoverDelta IndividualWithPendingDelta::apply_pending_delta() {
        pending_delta.apply_to(individual);
        CrossoverDelta delta = std::move(pending_delta);
        pending_delta = CrossoverDelta(*this);
        return delta;
    }
}