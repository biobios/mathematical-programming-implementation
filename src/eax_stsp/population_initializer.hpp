#pragma once

#include <cstddef>
#include <vector>
#include <random>
#include <format>
#include <fstream>

#include "utils.hpp"

namespace tsp {
    template <std::uniform_random_bit_generator RandomGen = std::mt19937, typename PostProcessFunc = mpi::NOP_Function>
    requires requires(PostProcessFunc post_process, std::vector<size_t> individual) {
        { post_process(individual) } -> std::convertible_to<void>;
    }
    class PopulationInitializer {
        public:
            PopulationInitializer(size_t population_size, size_t city_count, PostProcessFunc post_process = PostProcessFunc())
                : population_size_(population_size), city_count_(city_count), post_process_(post_process) {}
            ~PopulationInitializer() = default;
            
            std::vector<std::vector<size_t>> initialize_population(RandomGen::result_type seed, std::string cache_file) const
            {
                std::vector<std::vector<size_t>> population;
                population.reserve(population_size_);
                
                std::ifstream cache(cache_file);
                if (cache.is_open()) {
                    for (size_t i = 0; i < population_size_; ++i) {
                        std::vector<size_t> cities(city_count_);
                        cities.clear();
                        for (size_t j = 0; j < city_count_; ++j) {
                            size_t city;
                            cache >> city;
                            cities.push_back(city);
                        }
                        population.emplace_back(std::move(cities));
                    }
                    cache.close();
                    return population;
                }
                
                RandomGen rng(seed);
                
                for (size_t i = 0; i < population_size_; ++i) {
                    std::vector<size_t> cities(city_count_);
                    std::iota(cities.begin(), cities.end(), 0);
                    std::shuffle(cities.begin(), cities.end(), rng);
                    post_process_(cities);
                    population.emplace_back(std::move(cities));
                }
                
                std::ofstream out(cache_file);
                if (out.is_open()) {
                    for (const auto& cities : population) {
                        for (const auto& city : cities) {
                            out << city << " ";
                        }
                        out << std::endl;
                    }
                    out.close();
                } else {
                    throw std::runtime_error(std::format("Failed to open cache file: {}", cache_file));
                }
                
                return population;
            }
        private:
            size_t population_size_;
            size_t city_count_;
            PostProcessFunc post_process_;
    };
}