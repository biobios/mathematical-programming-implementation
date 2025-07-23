#pragma once

#include <vector>
#include <array>
#include <iostream>
#include <cmath>
#include <chrono>

namespace eax {
    class Child;

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
        Individual& operator=(const Child& child);
    private:
        std::vector<std::array<size_t, 2>> doubly_linked_list;
    };
    
    class Child {
    public:
        static std::array<double, 2> calc_times;
        static void reset_times() {
            calc_times = {0.0, 0.0};
        }
        static void print_times() {
            std::cout << "calc_distance time: " << calc_times[0] << " seconds, "
                      << "calc_entropy time: " << calc_times[1] << " seconds" << std::endl;
        }
        struct Modification {
            // (v1, v2)
            std::pair<size_t, size_t> edge1;
            size_t new_v2;
        };
        Child(const std::vector<Modification>& modifications) : modifications(modifications) {}
        Child(std::vector<Modification>&& modifications) : modifications(std::move(modifications)) {}
        void apply_to(Individual& individual) const {
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
        template <typename distance_t>
        distance_t get_delta_distance(const std::vector<std::vector<distance_t>>& adjacency_matrix) const {
            auto start_time = std::chrono::high_resolution_clock::now();
            distance_t delta = 0;
            for (const auto& modification : modifications) {
                auto [v1, v2] = modification.edge1;
                size_t new_v2 = modification.new_v2;
                delta += adjacency_matrix[v1][new_v2] - adjacency_matrix[v1][v2];
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            Child::calc_times[0] += std::chrono::duration<double>(end_time - start_time).count();
            return delta;
        }
        
        double get_delta_entropy(std::vector<std::vector<size_t>>& pop_edge_counts, size_t pop_size) const {
            auto start_time = std::chrono::high_resolution_clock::now();
            double delta_H = 0.0;

            auto calc_entropy = [](size_t count, size_t n) {
                if (count == 0) return 0.0;
                return -static_cast<double>(count) / n * std::log(static_cast<double>(count) / n);
            };

            for (const auto& modification : modifications) {
                auto [v1, v2] = modification.edge1;
                size_t new_v2 = modification.new_v2;
                
                // v1 -> v2が消えたなら
                delta_H += calc_entropy(pop_edge_counts[v1][v2] - 1, pop_size)
                         - calc_entropy(pop_edge_counts[v1][v2], pop_size);

                // v1 -> new_v2ができたなら
                delta_H += calc_entropy(pop_edge_counts[v1][new_v2] + 1, pop_size)
                         - calc_entropy(pop_edge_counts[v1][new_v2], pop_size);
                
                pop_edge_counts[v1][v2] -= 1;
                pop_edge_counts[v1][new_v2] += 1;
            }
            
            // もとに戻す
            for (auto it = modifications.crbegin(); it != modifications.crend(); ++it) {
                auto [v1, v2] = it->edge1;
                size_t new_v2 = it->new_v2;
                pop_edge_counts[v1][v2] += 1;
                pop_edge_counts[v1][new_v2] -= 1;
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            Child::calc_times[1] += std::chrono::duration<double>(end_time - start_time).count();
            return delta_H;
        }
    private:
        std::vector<Modification> modifications;
    };
    
    inline Individual& Individual::operator=(const Child& child) {
        child.apply_to(*this);
        return *this;
    }
}