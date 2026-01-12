#pragma once

#include <vector>
#include <random>

#include "utils.hpp"
#include "genetic_algorithm.hpp"

namespace eax {
    /**
     * @brief GAを実行するオブジェクト
     * @tparam GenerationalStep 1世代の世代交代を行う関数オブジェクト
     * @tparam UpdateFunc 更新処理を行い、世代交代を継続する場合、NotTerminatedを返す関数オブジェクト
     * @tparam LoggingFunc ロギングを行う関数オブジェクト (デフォルトは何もしない関数)
     * @tparam PostProcess 後処理を行う関数オブジェクト (デフォルトは何もしない関数)
     */
    template <typename GenerationalStep, typename UpdateFunc, typename LoggingFunc = mpi::NOP_Function, typename PostProcess = mpi::NOP_Function> 
    class GenerationalModel {
    public:
        /**
         * @brief デフォルトコンストラクタ
         * @note GenerationalStepとPostProcessはデフォルト構築可能である必要があります。
         */
        GenerationalModel()
            requires(std::default_initializable<GenerationalStep> && std::default_initializable<PostProcess>)
            = default;
        
        /**
         * @brief コンストラクタ
         * @param generational_step 1世代の世代交代を行う関数オブジェクト
         * @param post_process 後処理を行う関数オブジェクト (デフォルトは何もしない関数)
         */
        GenerationalModel(GenerationalStep generational_step, UpdateFunc update_func, LoggingFunc logging, PostProcess post_process = {}) 
            : generational_step(std::move(generational_step)), update_func(std::move(update_func)), logging(std::move(logging)), post_process(std::move(post_process)) {}

        template <typename Individual, typename Context>
        constexpr std::pair<mpi::genetic_algorithm::TerminationReason, std::vector<Individual>>
            execute(std::vector<Individual> population, Context& context, size_t generation = 0)
        {
            mpi::genetic_algorithm::TerminationReason reason = mpi::genetic_algorithm::TerminationReason::NotTerminated;
            while(reason == mpi::genetic_algorithm::TerminationReason::NotTerminated) {
                logging(population, context, generation);
                generational_step(population, context);
                generation++;
                reason = update_func(population, context, generation);
            }
            
            post_process(population, context, generation, reason);
            return {reason, std::move(population)};
        }
    private:
        GenerationalStep generational_step;
        UpdateFunc update_func;
        LoggingFunc logging;
        PostProcess post_process;
    };

    /**
     * @brief 1世代の世代交代を行う関数オブジェクト
     * @tparam FitnessFunc 適応度を計算する関数オブジェクト
     * @tparam CrossOverFunc 交叉を行う関数オブジェクト
     * @tparam RandomGen 乱数生成器の型
     */
    template <typename FitnessFunc, typename CrossOverFunc>
    class GenerationalStep
    {
    public:
        GenerationalStep(FitnessFunc fitness_func, CrossOverFunc cross_over_func)
            : fitness_func(std::move(fitness_func)), cross_over(std::move(cross_over_func)){}

        /**
         * @brief 世代交代モデルの１回の世代交代を実行する
         * @param population 集団
         * @param context 実行コンテキスト
         * @note contextはrandom_genメンバ変数を持ち、それはコンセプトstd::uniform_random_bit_generatorを満たす型である必要があります。
         */
        template <typename Individual, typename Context>
            requires(requires(std::vector<Individual> population, FitnessFunc fitness_func, CrossOverFunc cross_over, Context context) {
                { fitness_func(cross_over(population[0], population[1], context)[0], context) } -> std::convertible_to<double>;
                population[0] = cross_over(population[0], population[1], context)[0];
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

            std::vector<size_t> indices(population_size);
            std::iota(indices.begin(), indices.end(), 0);
            std::shuffle(indices.begin(), indices.end(), context.random_gen);

            population.push_back(population[indices[0]]); // 最初の個体を追加しておく
            indices.push_back(population.size() - 1);
            for (size_t i = 0; i < population_size; ++i) {
                size_t parent_A_index = indices[i];
                size_t parent_B_index = indices[(i + 1) % population_size];
                Individual& parent_A = population[parent_A_index];
                Individual& parent_B = population[parent_B_index];
                std::vector<Child> children = cross_over(parent_A, parent_B, context);

                if (children.empty()) {
                    continue; // 子供が生成されなかった場合はスキップ
                }

                std::vector<double> children_fitness = calc_all_fitness(children, context, fitness_func);
                
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
                    // 子供の適応度が0より大きい場合、親Aを子供に置き換える
                    parent_A = std::move(children[best_index]);
                } else {
                    // 子供の適応度が0以下の場合、親Aをそのままにする
                }
            }

            // 最後の個体を削除
            population.pop_back();
        }
    private:
        FitnessFunc fitness_func;
        CrossOverFunc cross_over;
    };
}