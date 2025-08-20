#pragma once

#include <cstddef>
#include <utility>

#include "eaxdef.hpp"

namespace eax {
class CrossoverDelta {
public:
    struct Modification {
        // (v1, v2)
        std::pair<size_t, size_t> edge1;
        size_t new_v2; // new vertex connected to v1
    };
    
    CrossoverDelta(std::vector<Modification>&& modifications)
        : modifications(std::move(modifications)) {}
    
    template <doubly_linked_list_like T>
    void apply_to(T& individual) const {
        for (const auto& modification : modifications) {
            auto [v1, v2] = modification.edge1;
            size_t new_v2 = modification.new_v2;
            if (individual[v1][0] == v2) {
                individual[v1][0] = new_v2;
            } else {
                individual[v1][1] = new_v2;
            }
        }
    }
private:
    std::vector<Modification> modifications;
};
}