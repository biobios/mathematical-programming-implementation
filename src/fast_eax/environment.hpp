#pragma once

#include <vector>
#include <functional>
#include <chrono>
#include <random>

#include "edge_counter.hpp"
#include "tsp_loader.hpp"
#include "object_pool.hpp"
#include "limited_range_integer_set.hpp"
#include "individual_with_pending_delta.hpp"

namespace eax {
    using Individual = IndividualWithPendingDelta;

    enum class EAXType {
        Rand,
        N_AB,
    };

    enum class SelectionType {
        Greedy,
        LDL,
        Ent,
    };

    struct Environment {
        tsp::TSP tsp;
        size_t N_parameter;
        size_t population_size;
        EAXType eax_type;
        SelectionType selection_type;
        EdgeCounter<> pop_edge_counts; // 各エッジの個数
        std::mt19937 random_gen;
    };
}