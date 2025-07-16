#include "individual.hpp"

namespace eax {
    Individual::Individual(const std::vector<size_t>& path) {
        doubly_linked_list.resize(path.size());
        for (size_t i = 0; i < path.size(); ++i) {
            size_t prev_index = (i == 0) ? path.size() - 1 : i - 1;
            size_t next_index = (i + 1) % path.size();
            doubly_linked_list[path[i]][0] = path[prev_index];
            doubly_linked_list[path[i]][1] = path[next_index];
        }
    }
    
    size_t Individual::size() const {
        return doubly_linked_list.size();
    }
}