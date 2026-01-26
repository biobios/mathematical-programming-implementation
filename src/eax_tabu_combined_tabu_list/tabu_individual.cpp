#include "tabu_individual.hpp"

TabuIndividual &TabuIndividual::operator=(const eax::CrossoverDelta &delta)
{
    pending_delta = delta;
    return *this;
}

TabuIndividual &TabuIndividual::operator=(eax::CrossoverDelta &&delta)
{
    pending_delta = std::move(delta);
    return *this;
}

std::vector<std::pair<size_t, size_t>> const& TabuIndividual::get_tabu_edges() const
{
    return tabu_edges[current_tabu_index];
}
