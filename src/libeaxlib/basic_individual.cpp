#include "basic_individual.hpp"

namespace eax {
BasicIndividual::BasicIndividual(const std::vector<size_t>& path, const adjacency_matrix_t& adjacency_matrix)
    : doubly_linked_list(path.size()) {
    
    size_t city_count = path.size();
    for (size_t i = 1; i < city_count - 1; ++i) {
        size_t current_city = path[i];
        size_t prev_city = path[i - 1];
        size_t next_city = path[i + 1];

        doubly_linked_list[current_city] = {prev_city, next_city};
        
        distance += adjacency_matrix[current_city][prev_city];
    }
    
    // 最初の都市
    size_t first_city = path[0];
    size_t second_city = path[1];
    size_t last_city = path[city_count - 1];
    doubly_linked_list[first_city] = {last_city, second_city};
    distance += adjacency_matrix[first_city][last_city];

    // 最後の都市
    size_t penultimate_city = path[city_count - 2];
    doubly_linked_list[last_city] = {penultimate_city, first_city};
    distance += adjacency_matrix[last_city][penultimate_city];
}
}