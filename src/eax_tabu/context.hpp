#pragma once

#include <vector>
#include <functional>
#include <chrono>
#include <random>

#include "tsp_loader.hpp"
#include "object_pool.hpp"
#include "limited_range_integer_set.hpp"
#include "individual.hpp"
#include "eax_tabu.hpp"

namespace eax {
    enum class SelectionType {
        Greedy,
        Ent,
        DistancePreserving,
    };

    enum class EAXType {
        EAX_Rand
    };
    
    struct EAX_n_AB {
        size_t n;
        
        // コンストラクタ
        EAX_n_AB(size_t n) : n(n) {}
        EAX_n_AB(const std::string& str) {
            if (!is_EAX_N_AB(str)) {
                throw std::runtime_error("Invalid EAX_N_AB string: " + str);
            }

            std::string n_str = str.substr(4, str.size() - 7);
            n = std::stoul(n_str);
        }
        // EAX_N_ABとして解釈可能な文字列か否か
        static bool is_EAX_N_AB(const std::string& str) {
            if (!str.starts_with("EAX_") || !str.ends_with("_AB") || str.size() <= 7) {
                return false;
            }
            
            std::string n_str = str.substr(4, str.size() - 7);
            for (char c : n_str) {
                if (!std::isdigit(c)) return false;
            }
            return true;
        }
    };
    
    using eax_type_t = std::variant<EAXType, EAX_n_AB>;

    
    struct Environment {
        tsp::TSP tsp;
        size_t population_size;
        size_t num_children;
        SelectionType selection_type;
        std::mt19937::result_type random_seed;
        eax_type_t eax_type;
    };

    struct Context {
        Environment env;

        std::vector<std::vector<size_t>> pop_edge_counts; // 各エッジの個数
        std::mt19937 random_gen;

        // 最良解の長さ
        size_t best_length = 1e18;
        // 最良解に到達した世代
        size_t generation_of_reached_best = 0;
        // 停滞した世代数
        size_t stagnation_generations = 0;
        // 現在の世代数
        size_t current_generation = 0;
        // 最終世代
        size_t final_generation = 0;
        
        // 計測開始時刻 (これはシリアライズされない)
        std::chrono::system_clock::time_point start_time;
        // 経過時間
        double elapsed_time = 0.0;

        void set_initial_edge_counts(const std::vector<Individual>& init_pop) {
            pop_edge_counts.assign(env.tsp.city_count, std::vector<size_t>(env.tsp.city_count, 0));
            
            for (const auto& individual : init_pop) {
                for (size_t i = 0; i < individual.size(); ++i) {
                    size_t v1 = individual[i][0];
                    size_t v2 = individual[i][1];
                    pop_edge_counts[i][v1] += 1;
                    pop_edge_counts[i][v2] += 1;
                }
            }
        };
        
        void serialize(std::ostream& os) const;
        static Context deserialize(std::istream& is, tsp::TSP tsp);
    };
}