#pragma once

#include <cstdint>

namespace mpi
{
    namespace genetic_algorithm
    {
        class GenerationsNumEndCondition
        {
        public:
            GenerationsNumEndCondition(std::size_t end_generations_num) noexcept
                : end_generations_num(end_generations_num), current_generation(0) {}

            template <typename... Args>
            constexpr bool operator()(Args&&... args) noexcept
            {
                return this->current_generation++ >= this->end_generations_num;
            }
            
        private:
            std::size_t end_generations_num;
            std::size_t current_generation;

        };
    }
}