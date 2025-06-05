#pragma once
#include <vector>
#include <random>

#include "utils.hpp"

namespace mpi
{
    namespace genetic_algorithm
    {
        struct SimpleGAImpl
        {
            /**
             * @brief SimpleGAを実行する関数
             * @param population 初期集団
             * @param end_condition 終了条件を満たすかどうかを判定する関数(オブジェクト)
             * @param fitness_func 適応度を計算する関数(オブジェクト)
             * @param cross_over 交叉を行う関数(オブジェクト)
             * @param env 環境情報
             * @param rng 乱数生成器
             * @param mutate 突然変異を行う関数(オブジェクト) (デフォルトは何もしない関数)
             * @return 最終的な集団
             */
            template <typename Individual, typename EndCondition, typename FitnessFunc, typename Environment, typename CrossOverFunc, std::uniform_random_bit_generator RandomGen, typename MutationFunc = NOP_Function>
                requires(requires(std::vector<Individual> population, EndCondition end_condition, FitnessFunc fitness_func, CrossOverFunc cross_over, Environment env, RandomGen rng, MutationFunc mutate, std::vector<double> fitness_values) {
                    { end_condition(population, fitness_values, env) } -> std::convertible_to<bool>;
                    { fitness_func(population[0], env) } -> std::convertible_to<double>;
                    { cross_over(population[0], population[1], 1, env, rng) } -> std::convertible_to<std::vector<Individual>>;
                    mutate(population[0], env, rng);
                })
            constexpr std::vector<Individual> operator()(std::vector<Individual> population, EndCondition end_condition, FitnessFunc fitness_func, CrossOverFunc cross_over, Environment env, RandomGen rng, MutationFunc mutate = {}) const
            {
                auto calc_all_fitness = [&fitness_func](const std::vector<Individual>& pop, const Environment& env) {
                    std::vector<double> fitness_values(pop.size());
                    for (size_t i = 0; i < pop.size(); ++i) {
                        fitness_values[i] = fitness_func(pop[i], env);
                    }
                    return fitness_values;
                };

                std::vector<double> fitness_values = calc_all_fitness(population, env);
                while (!end_condition(population, fitness_values, env)) {
                    std::vector<Individual> new_population;
                    new_population.reserve(population.size());
                    
                    // ルーレット選択を使用して親を選択
                    std::discrete_distribution<size_t> dist(fitness_values.begin(), fitness_values.end());
                    for (size_t i = 0; i < population.size(); ++i) {
                        // 親を選択
                        size_t parent1_index = dist(rng);
                        size_t parent2_index = dist(rng);
                        
                        // 交叉
                        Individual child = cross_over(population[parent1_index], population[parent2_index], 1, env, rng)[0];
                        
                        // 突然変異
                        mutate(child, env, rng);
                        
                        new_population.push_back(std::move(child));
                    }
                    // 新しい集団を現在の集団に置き換え
                    population = std::move(new_population);
                    // 適応度を再計算
                    fitness_values = calc_all_fitness(population, env);
                }
                
                return population;
            }
        };
        
        constexpr SimpleGAImpl SimpleGA = {};
    }
}