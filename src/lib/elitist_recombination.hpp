#pragma once

#include <vector>
#include <random>

#include "utils.hpp"

namespace mpi
{
    namespace genetic_algorithm
    {
        template <size_t N_cross>
        struct ElitistRecombinationImpl
        {
            /**
             * @brief ElitistRecombinationを実行する関数
             * @param population 初期集団
             * @param end_condition 終了条件を満たすかどうかを判定する関数(オブジェクト)
             * @param fitness_func 適応度を計算する関数(オブジェクト)
             * @param cross_over 交叉を行う関数(オブジェクト)
             * @param env 環境情報
             * @param rng 乱数生成器
             * @param logging ロギング関数(オブジェクト) (デフォルトは何もしない関数)
             * @return 最終的な集団
             */
            template <typename Individual, typename EndCondition, typename FitnessFunc, typename Environment, typename CrossOverFunc, std::uniform_random_bit_generator RandomGen, typename LoggingFunc = NOP_Function>
                requires(requires(std::vector<Individual> population, EndCondition end_condition, FitnessFunc fitness_func, CrossOverFunc cross_over, Environment env, RandomGen rng, size_t generation, std::vector<double> fitness_values, LoggingFunc logging) {
                    { end_condition(population, fitness_values, env, generation) } -> std::convertible_to<bool>;
                    { fitness_func(population[0], env) } -> std::convertible_to<double>;
                    { cross_over(population[0], population[1], 1, env, rng) } -> std::convertible_to<std::vector<Individual>>;
                    { logging(population, fitness_values, env, generation) } -> std::convertible_to<void>;
                })
            constexpr std::vector<Individual> operator()(std::vector<Individual> population, EndCondition end_condition, FitnessFunc fitness_func, CrossOverFunc cross_over, Environment env, RandomGen rng, LoggingFunc&& logging = {}) const
            {
                auto calc_all_fitness = [&fitness_func](const std::vector<Individual>& pop, const Environment& env) {
                    std::vector<double> fitness_values(pop.size());
                    for (size_t i = 0; i < pop.size(); ++i) {
                        fitness_values[i] = fitness_func(pop[i], env);
                    }
                    return fitness_values;
                };
                
                size_t generation = 0;

                size_t population_size = population.size();
                std::vector<double> fitness_values = calc_all_fitness(population, env);
                while (!end_condition(population, fitness_values, env, generation)) {
                    // ロギング
                    logging(population, fitness_values, env, generation);

                    // ランダムにペアを作成
                    std::vector<size_t> indices(population_size);
                    std::iota(indices.begin(), indices.end(), 0);
                    std::shuffle(indices.begin(), indices.end(), rng);
                    
                    for (size_t i = 1; i < population_size; i += 2) {
                        // ペアを選択
                        size_t parent1_index = indices[i - 1];
                        size_t parent2_index = indices[i];
                        
                        // 交叉
                        std::vector<Individual> children = cross_over(population[parent1_index], population[parent2_index], N_cross, env, rng);
                        std::vector<double> children_fitness = calc_all_fitness(children, env);
                        
                        // 親と子どもの集団から最良の２個体を選択
                        children.emplace_back(move(population[parent1_index]));
                        children.emplace_back(move(population[parent2_index]));
                        children_fitness.push_back(fitness_values[parent1_index]);
                        children_fitness.push_back(fitness_values[parent2_index]);
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
                        fitness_values[parent1_index] = best_fitness;
                        fitness_values[parent2_index] = second_best_fitness;
                    }
                    
                    generation++;
                }
                
                return population;
            }
        };
        
        template <size_t N_cross>
        constexpr ElitistRecombinationImpl<N_cross> ElitistRecombination;
    }
}