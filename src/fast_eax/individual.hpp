#pragma once

#include <vector>
#include <array>

namespace eax {
    class Individual {
    public:
        Individual(const std::vector<size_t>& path);
        constexpr std::array<size_t, 2>& operator[](size_t index) {
            return doubly_linked_list[index];
        }
        constexpr std::array<size_t, 2> const& operator[](size_t index) const {
            return doubly_linked_list[index];
        }
        size_t size() const;
        std::vector<size_t> to_path() const;
    private:
        std::vector<std::array<size_t, 2>> doubly_linked_list;
    };
}