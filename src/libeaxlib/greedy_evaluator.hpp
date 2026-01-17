#pragma once

#include "crossover_delta.hpp"

namespace eax {
namespace eval {
namespace delta {
namespace impl {
struct Greedy {
    double operator()(const CrossoverDelta& child) const {
        return -1.0 * child.get_delta_distance();
    }
};
}

constexpr impl::Greedy Greedy{};

}
}
}