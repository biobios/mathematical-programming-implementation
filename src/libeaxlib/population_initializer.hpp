#pragma once

#include <cstddef>
#include <vector>
#include <random>
#include <format>
#include <fstream>

#include "utils.hpp"

namespace tsp {
    template <std::uniform_random_bit_generator RandomGen = std::mt19937>
    class PopulationInitializer {
        public:
            PopulationInitializer(size_t population_size, size_t city_count)
                : population_size_(population_size), city_count_(city_count) {}
            ~PopulationInitializer() = default;
            
            template <typename PostProcessFunc = mpi::NOP_Function>
                requires std::invocable<PostProcessFunc, std::vector<size_t>&>
            std::vector<std::vector<size_t>> initialize_population(RandomGen::result_type seed, std::string cache_file, PostProcessFunc&& post_process = {}) const
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
                    post_process(cities);
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
    };
}