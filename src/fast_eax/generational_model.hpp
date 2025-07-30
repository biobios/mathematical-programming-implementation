#pragma once

#include <vector>
#include <random>

#include "utils.hpp"

namespace eax {
    
    template <size_t N_cross>
    struct GenerationalModelImpl
    {
        /**
        * @brief 世代交代モデルを実装する関数オブジェクト
        * @param population 初期集団
        * @param update_func 更新処理を行い、世代交代を継続する場合、trueを返す関数(オブジェクト)
        * @param fitness_func 適応度を計算する関数(オブジェクト)
        * @param cross_over 交叉を行う関数(オブジェクト)
        * @param env 環境情報
        * @param rng 乱数生成器
        * @param logging ロギング関数(オブジェクト) (デフォルトは何もしない関数)
        * @return 最終的な集団
        */
        template <typename Individual, typename UpdateFunc, typename FitnessFunc, typename Environment, typename CrossOverFunc, std::uniform_random_bit_generator RandomGen, typename LoggingFunc = mpi::NOP_Function>
            requires(requires(std::vector<Individual> population, UpdateFunc update_func, FitnessFunc fitness_func, CrossOverFunc cross_over, Environment env, RandomGen rng, size_t generation, LoggingFunc logging) {
                { update_func(population, env, generation) } -> std::convertible_to<bool>;
                { fitness_func(cross_over(population[0], population[1], 1, env, rng)[0], env) } -> std::convertible_to<double>;
                population[0] = cross_over(population[0], population[1], 1, env, rng)[0];
                { logging(population, env, generation) } -> std::convertible_to<void>;
            })
        constexpr std::vector<Individual> operator()(std::vector<Individual> population, UpdateFunc update_func, FitnessFunc fitness_func, CrossOverFunc cross_over, Environment env, RandomGen rng, LoggingFunc&& logging = {}) const
        {
            using Child = std::invoke_result_t<CrossOverFunc, Individual&, Individual&, size_t, Environment&, RandomGen&>::value_type;
            auto calc_all_fitness = [&fitness_func](const std::vector<Child>& children, Environment& env) {
                std::vector<double> fitness_values(children.size());
                for (size_t i = 0; i < children.size(); ++i) {
                    fitness_values[i] = fitness_func(children[i], env);
                }
                return fitness_values;
            };
            
            size_t generation = 0;

            size_t population_size = population.size();
            while (update_func(population, env, generation)) {
                // ロギング
                logging(population, env, generation);
                
                std::vector<size_t> indices(population_size);
                std::iota(indices.begin(), indices.end(), 0);
                std::shuffle(indices.begin(), indices.end(), rng);

                population.push_back(population[indices[0]]); // 最初の個体を追加しておく
                indices.push_back(population.size() - 1);
                for (size_t i = 0; i < population_size; ++i) {
                    size_t parent_A_index = indices[i];
                    size_t parent_B_index = indices[(i + 1) % population_size];
                    Individual& parent_A = population[parent_A_index];
                    Individual& parent_B = population[parent_B_index];
                    std::vector<Child> children = cross_over(parent_A, parent_B, N_cross, env, rng);

                    if (children.empty()) {
                        continue; // 子供が生成されなかった場合はスキップ
                    }

                    std::vector<double> children_fitness = calc_all_fitness(children, env);
                    
                    // 子供の中で最良の個体を選択
                    size_t best_index = 0;
                    double best_fitness = children_fitness[0];
                    for (size_t j = 1; j < children_fitness.size(); ++j) {
                        if (children_fitness[j] > best_fitness) {
                            best_fitness = children_fitness[j];
                            best_index = j;
                        }
                    }
                    
                    if (best_fitness > 0.0) {
                        // 子供の適応度が0でない場合、親Aを子供に置き換える
                        parent_A = std::move(children[best_index]);
                    } else {
                        // 子供の適応度が0の場合、親Aをそのままにする
                    }
                }

                // 最後の個体を削除
                population.pop_back();
                
                generation++;
            }
            
            return population;
        }
    };
    
    template <size_t N_cross>
    constexpr GenerationalModelImpl<N_cross> GenerationalModel;
}