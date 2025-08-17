#pragma once

#include <vector>
#include <array>
#include <iostream>
#include <cmath>
#include <chrono>

namespace eax {
    class Individual;

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
        Child() = default;
        Child(const std::vector<Modification>& modifications) : modifications(modifications) {}
        Child(std::vector<Modification>&& modifications) : modifications(std::move(modifications)) {}
        void apply_to(Individual& individual) const;

        void update_edge_counts(std::vector<std::vector<size_t>>& pop_edge_counts) const {
            for (const auto& modification : modifications) {
                auto [v1, v2] = modification.edge1;
                size_t new_v2 = modification.new_v2;
                
                // v1 -> v2が消えたなら
                pop_edge_counts[v1][v2] -= 1;

                // v1 -> new_v2ができたなら
                pop_edge_counts[v1][new_v2] += 1;
            }
        }

        int64_t get_delta_distance(const std::vector<std::vector<int64_t>>& adjacency_matrix) const {
            if (delta_distance_calculated) {
                return delta_distance;
            }
            auto start_time = std::chrono::high_resolution_clock::now();
            int64_t delta = 0;
            for (const auto& modification : modifications) {
                auto [v1, v2] = modification.edge1;
                size_t new_v2 = modification.new_v2;
                delta += adjacency_matrix[v1][new_v2] - adjacency_matrix[v1][v2];
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            Child::calc_times[0] += std::chrono::duration<double>(end_time - start_time).count();
            
            delta_distance = delta / 2;
            delta_distance_calculated = true;
            return delta_distance;
        }
        
        double get_delta_entropy(std::vector<std::vector<size_t>>& pop_edge_counts, size_t pop_size) const {
            if (delta_entropy_calculated) {
                return delta_entropy;
            }
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
            delta_entropy = delta_H;
            delta_entropy_calculated = true;
            return delta_entropy;
        }
        
        size_t size() const {
            return modifications.size();
        }
    private:
        std::vector<Modification> modifications;
        mutable bool delta_distance_calculated = false;
        mutable int64_t delta_distance = 0;
        mutable bool delta_entropy_calculated = false;
        mutable double delta_entropy = 0.0;
    };

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
        Individual& operator=(const Child& child) {
            prev_diff = child;
            return *this;
        }
        
        Child update(const std::vector<std::vector<int64_t>>& adjacency_matrix) {
            Child child = std::move(prev_diff);
            prev_diff = Child();
            child.apply_to(*this);
            distance += child.get_delta_distance(adjacency_matrix);
            return child;
        }
    private:
        std::vector<std::array<size_t, 2>> doubly_linked_list;
        Child prev_diff;
        int64_t distance = 0;
    };

    inline void Child::apply_to(Individual& individual) const {
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
    
}