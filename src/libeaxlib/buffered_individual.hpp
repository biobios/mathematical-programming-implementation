#pragma once

#include "eaxdef.hpp"
#include "delta_with_individual.hpp"
#include "basic_individual.hpp"

namespace eax {
class BufferedIndividual : public ReadableWithBasicIndividual<BufferedIndividual> {
public:
    using delta_t = DeltaWithIndividual<BasicIndividual>;

    BufferedIndividual(const std::vector<size_t>& path, const adjacency_matrix_t& adjacency_matrix)
        : ReadableWithBasicIndividual<BufferedIndividual>(path, adjacency_matrix),
          buffered_individual(this->individual) {}

    BufferedIndividual& operator=(const delta_t& delta_view);

    operator BasicIndividual const&() const {
        return individual;
    }

    /**
     * @brief バッファを書き込む
     */
    void flush_buffer() {
        individual = buffered_individual;
    }

private:
    BasicIndividual buffered_individual;
};

static_assert(individual_readable<BufferedIndividual>);
}