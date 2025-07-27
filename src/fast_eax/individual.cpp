#include "individual.hpp"

namespace eax {
    Individual::Individual(const std::vector<size_t>& path, const std::vector<std::vector<int64_t>>& adjacency_matrix) {
        doubly_linked_list.resize(path.size());
        for (size_t i = 1; i < path.size() - 1; ++i) {
            size_t prev_index = i - 1;
            size_t next_index = i + 1;
            doubly_linked_list[path[i]][0] = path[prev_index];
            doubly_linked_list[path[i]][1] = path[next_index];
            distance += adjacency_matrix[path[i]][path[next_index]];
        }
        
        doubly_linked_list[path[0]][0] = path.back();
        doubly_linked_list[path[0]][1] = path[1];
        distance += adjacency_matrix[path[0]][path[1]];
        doubly_linked_list[path.back()][0] = path[path.size() - 2];
        doubly_linked_list[path.back()][1] = path[0];
        distance += adjacency_matrix[path.back()][path[0]];
    }
    
    size_t Individual::size() const {
        return doubly_linked_list.size();
    }

    std::vector<size_t> Individual::to_path() const
    {
        std::vector<size_t> path;
        size_t current_city = 0;
        size_t prev_city = 0;
        for (size_t i = 0; i < doubly_linked_list.size(); ++i) {
            path.push_back(current_city);
            size_t next_city = doubly_linked_list[current_city][0];
            if (next_city == prev_city) {
                next_city = doubly_linked_list[current_city][1];
            }
            prev_city = current_city;
            current_city = next_city;
        }
        return path;
    }
}