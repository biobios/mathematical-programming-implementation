#pragma once

#include <vector>
#include <random>

#include "utils.hpp"

namespace mpi
{
    namespace genetic_algorithm
    {
        template <typename FitnessFunc, typename CrossOverFunc>
        struct ElitistRecombination {
            ElitistRecombination(FitnessFunc fitness_func, CrossOverFunc cross_over)
                : fitness_func(std::move(fitness_func)), cross_over(std::move(cross_over)) {}
            
            template <typename Individual, typename Context>
                requires(requires(std::vector<Individual> population, FitnessFunc fitness_func, CrossOverFunc cross_over, Context context) {
                    { fitness_func(cross_over(population[0], population[1], context)[0], context) } -> std::convertible_to<double>;
                    population[0] = std::move(cross_over(population[0], population[1], context)[0]);
                    cross_over(population[0], population[1], context).emplace_back(population[0]); // 個体から子個体を生成できること
                    context.random_gen;
                } && std::uniform_random_bit_generator<decltype(Context::random_gen)>)
            void operator()(std::vector<Individual>& population, Context& context)
            {
                using Child = std::invoke_result_t<CrossOverFunc, Individual&, Individual&, Context&>::value_type;
                
                auto calc_all_fitness = [](const std::vector<Child>& children, Context& context, FitnessFunc& fitness_func) {
                    std::vector<double> fitness_values(children.size());
                    for (size_t i = 0; i < children.size(); ++i) {
                        fitness_values[i] = fitness_func(children[i], context);
                    }
                    return fitness_values;
                };

                size_t population_size = population.size();

                if (population_size < 2) {
                    return; // 集団サイズが2未満の場合は何もしない
                }

                std::vector<size_t> indices(population_size);
                std::iota(indices.begin(), indices.end(), 0);
                std::shuffle(indices.begin(), indices.end(), context.random_gen);

                for (size_t i = 1; i < population_size; i += 2) {
                    // ペアを選択
                    size_t parent1_index = indices[i - 1];
                    size_t parent2_index = indices[i];
                    auto& parent1 = population[parent1_index];
                    auto& parent2 = population[parent2_index];
                    
                    // 交叉
                    std::vector<Child> children = cross_over(parent1, parent2, context);

                    // 親を子どもの集団に追加
                    children.emplace_back(parent1);
                    children.emplace_back(parent2);

                    // 適応度計算
                    std::vector<double> children_fitness = calc_all_fitness(children, context, fitness_func);
                    
                    // 最良の２個体を選択
                    size_t best_index = 0;
                    size_t second_best_index = 0;
                    double best_fitness = -1.0;
                    double second_best_fitness = -1.0;
                    for (size_t j = 0; j < children_fitness.size(); ++j) {
                        if (children_fitness[j] > best_fitness) {
                            second_best_fitness = best_fitness;
                            second_best_index = best_index;
                            best_fitness = children_fitness[j];
                            best_index = j;
                        }else if (children_fitness[j] > second_best_fitness) {
                            second_best_fitness = children_fitness[j];
                            second_best_index = j;
                        }
                    }

                    population[parent1_index] = std::move(children[best_index]);
                    population[parent2_index] = std::move(children[second_best_index]);
                }
            }
        private:
            FitnessFunc fitness_func;
            CrossOverFunc cross_over;
        };
    }
}