#include "buffered_individual.hpp"

namespace eax {
BufferedIndividual &BufferedIndividual::operator=(const delta_t &delta_view)
{
    delta_view.apply_to(buffered_individual);
    return *this;
}
}