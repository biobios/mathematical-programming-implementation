#include "soft_two_opt.hpp"

#include <iostream>
#include <chrono>

namespace {
    void inversion(std::vector<size_t>& tour, std::vector<size_t>& pos, size_t left, size_t right) {
        if(left <= right) {
            for(size_t i = 0; i < (right - left + 1) / 2; ++i) {
                std::swap(tour[left + i], tour[right - i]);
                pos[tour[left + i]] = left + i;
                pos[tour[right - i]] = right - i;
            }
        } else {
            for(size_t i = 0; i < (tour.size() - (left - right) + 1) / 2; ++i) {
                std::swap(tour[(left + i) % tour.size()], tour[(right - i + tour.size()) % tour.size()]);
                pos[tour[(left + i) % tour.size()]] = (left + i) % tour.size();
                pos[tour[(right - i + tour.size()) % tour.size()]] = (right - i + tour.size()) % tour.size();
            }
        }
    }

    void apply_soft_2opt(
        std::vector<size_t>& path,
        const std::vector<std::vector<int64_t>>& distance_matrix,
        const std::vector<std::vector<std::pair<int64_t, size_t>>>& nearest_neighbors,
        size_t near_range
    ) {
        const size_t n = path.size();
        std::vector<size_t> tour = path;
        std::vector<size_t> pos(n);
        for (size_t i = 0; i < n; ++i) {
            pos[tour[i]] = i;
        }

        double improve = 1.;
        while (improve > 0.) {
            improve = 0.;
            for(size_t i = 1; i < n; ++i) {
                std::pair<size_t, size_t> swap_pos;
                size_t u1 = tour[i];
                size_t v1 = tour[(i + 1) % n];
                for(size_t d = 0; d < near_range; ++d) {
                    size_t u2 = tour[pos[nearest_neighbors[u1][d].second]];
                    size_t v2 = tour[(pos[nearest_neighbors[u1][d].second] + 1) % n];
                    if((distance_matrix[u1][v1] + distance_matrix[u2][v2]) - (distance_matrix[u1][u2] + distance_matrix[v1][v2]) > improve) {
                        improve = (distance_matrix[u1][v1] + distance_matrix[u2][v2]) - (distance_matrix[u1][u2] + distance_matrix[v1][v2]);
                        swap_pos = std::make_pair((i + 1) % n, pos[nearest_neighbors[u1][d].second]);
                    }
                }
                if(swap_pos != std::pair<size_t, size_t>()) {
                    inversion(tour, pos, swap_pos.first, swap_pos.second);
                }
            }
        }
        path = tour;
    }

    double time_a = 0.0;
    
}

namespace eax {

void print_soft_2opt_time() {
    std::cout << "Time: " << time_a << " seconds" << std::endl;
}

//TODO: SoftTwoOptの実装
SoftTwoOpt::SoftTwoOpt(const std::vector<std::vector<int64_t>>& distance_matrix, const std::vector<std::vector<std::pair<int64_t, size_t>>>& nearest_neighbors, size_t near_range)
    : distance_matrix(distance_matrix), nearest_neighbors(nearest_neighbors), near_range(near_range)
{}

void SoftTwoOpt::apply(std::vector<size_t>& path)
{
    auto start_time = std::chrono::high_resolution_clock::now();

    apply_soft_2opt(path, distance_matrix, nearest_neighbors, near_range);

    auto end_time = std::chrono::high_resolution_clock::now();
    time_a += std::chrono::duration<double>(end_time - start_time).count();
}

}