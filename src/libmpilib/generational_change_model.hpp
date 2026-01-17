#pragma once

#include <vector>

#include "utils.hpp"
#include "genetic_algorithm.hpp"

namespace mpi {
/**
 * @brief GAを実行するオブジェクト
 * @tparam GenerationalStep 1世代の世代交代を行う関数オブジェクト
 * @tparam UpdateFunc 更新処理を行い、世代交代を継続する場合、NotTerminatedを返す関数オブジェクト
 * @tparam LoggingFunc ロギングを行う関数オブジェクト (デフォルトは何もしない関数)
 * @tparam PostProcess 後処理を行う関数オブジェクト (デフォルトは何もしない関数)
 */
template <typename GenerationalStep, typename UpdateFunc, typename LoggingFunc = mpi::NOP_Function, typename PostProcess = mpi::NOP_Function> 
class GenerationalChangeModel {
public:
    /**
     * @brief コンストラクタ
     * @param generational_step 1世代の世代交代を行う関数オブジェクト
     * @param update_func 更新処理を行う関数オブジェクト
     * @param logging ロギングを行う関数オブジェクト (デフォルトは何もしない関数)
     * @param post_process 後処理を行う関数オブジェクト (デフォルトは何もしない関数)
     */
    GenerationalChangeModel(GenerationalStep generational_step, UpdateFunc update_func, LoggingFunc logging = {}, PostProcess post_process = {}) 
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
}