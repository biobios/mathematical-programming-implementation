#pragma once

#include <vector>
#include <array>
#include <iostream>
#include <cmath>
#include <chrono>

#include "crossover_delta.hpp"

namespace eax {
    class Context;

    class Individual {
    public:
        Individual() = default;
        Individual(const std::vector<size_t>& path, const std::vector<std::vector<int64_t>>& adjacency_matrix);
        constexpr std::array<size_t, 2>& operator[](size_t index) {
            return doubly_linked_list[index];
        }

        constexpr std::array<size_t, 2> const& operator[](size_t index) const {
            return doubly_linked_list[index];
        }

        size_t size() const;
        size_t get_distance() const {
            return distance;
        }
        std::vector<size_t> to_path() const;
        void update_from(const CrossoverDelta& delta, Context& context);
        
        void serialize(std::ostream& os) const;
        static Individual deserialize(std::istream& is);
    private:
        std::vector<std::array<size_t, 2>> doubly_linked_list;
        int64_t distance = 0;
    };
}