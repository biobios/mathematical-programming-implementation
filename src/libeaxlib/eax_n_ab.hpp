#pragma once

#include <vector>
#include <memory>
#include <random>
#include <ranges>

#include "object_pool.hpp"

#include "eaxdef.hpp"
#include "object_pools.hpp"
#include "tsp_loader.hpp"
#include "eax_normal.hpp"

namespace eax {
class N_AB_e_set_assembler {
public:
    N_AB_e_set_assembler(size_t ab_cycle_count, size_t N_parameter, mpi::ObjectPool<std::vector<size_t>>&& any_size_vector_pool, std::mt19937& rng)
        : ab_cycle_count(ab_cycle_count),
            N_parameter(N_parameter),
            any_size_vector_pool(std::move(any_size_vector_pool)),
            indices_of_remaining_AB_cycles(this->any_size_vector_pool.acquire_unique()) {

        auto& indices = *indices_of_remaining_AB_cycles;
        indices.clear();
        indices.resize(ab_cycle_count);

        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);
    }
    
    bool has_next() const {
        if (N_parameter == 1 || ab_cycle_count <= N_parameter) {
            // シャッフルしなおしても必ず同じ組み合わせになる場合、
            // 残りのサイクルを使いきったら終了
            return indices_of_remaining_AB_cycles->size() > 0;
        }
        
        // シャッフルしなおすことで別の組み合わせが生成可能な場合、常に真
        return true;
    }
    
    mpi::pooled_unique_ptr<std::vector<size_t>> next(std::mt19937& rng) {
        auto& indices = *indices_of_remaining_AB_cycles;
        
        if (indices.size() < N_parameter && ab_cycle_count > N_parameter && N_parameter > 1) {
            // 残りのサイクルがN_parameter未満で、かつ、シャッフルしなおすことで別の組み合わせを生成可能な場合
            indices.resize(ab_cycle_count);
            std::iota(indices.begin(), indices.end(), 0);
            std::shuffle(indices.begin(), indices.end(), rng);
        }

        auto selected_AB_cycles_indices_ptr = any_size_vector_pool.acquire_unique();
        auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
        selected_AB_cycles_indices.clear();

        for (size_t i = 0; i < N_parameter && !indices.empty(); ++i) {
            size_t index = indices.back();
            indices.pop_back();
            
            selected_AB_cycles_indices.push_back(index);
        }
        return selected_AB_cycles_indices_ptr;
    }
private:
    size_t ab_cycle_count;
    size_t N_parameter;
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;

    mpi::pooled_unique_ptr<std::vector<size_t>> indices_of_remaining_AB_cycles;
};

class N_AB_e_set_assembler_builder {
public:
    N_AB_e_set_assembler_builder(ObjectPools& object_pools) :
        any_size_vector_pool(object_pools.any_size_vector_pool.share()) {}

    N_AB_e_set_assembler build(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles, const auto&, const auto&, size_t, const tsp::TSP&, std::mt19937& rng, size_t N_parameter) {
        return N_AB_e_set_assembler(AB_cycles.size(), N_parameter, any_size_vector_pool.share(), rng);
    }

    static size_t calc_AB_cycle_need(const auto&, const auto&, size_t children_size, const tsp::TSP&, std::mt19937&, size_t N_parameter) {
        return N_parameter * children_size;
    }

    template <typename CompletenessCategory>
    static constexpr bool is_available(CompletenessCategory) {
        // 完全なABサイクル集合を必要としないため、常にtrueを返す
        return true;
    }
private:
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
};

using EAX_N_AB = EAX_normal<N_AB_e_set_assembler_builder>;

class EAX_n_AB_tag {
public:
    static bool match_string(const std::string& str) {
        std::string start_str = "EAX_";
        std::string end_str = "_AB";
        if (!str.starts_with(start_str) || !str.ends_with(end_str)) {
            return false;
        }
        std::string n_str = str.substr(start_str.size(), str.size() - (start_str.size() + end_str.size()));
        try {
            size_t n_value = std::stoul(n_str);
            return n_value > 0;
        } catch (...) {
            return false;
        }
    }

    EAX_n_AB_tag() : n(1) {}
    EAX_n_AB_tag(const std::string& str)
        : n(parse_n(str)) {
        if (n == 0) {
            throw std::invalid_argument("N parameter must be greater than 0.");
        }
    }

    EAX_n_AB_tag(std::size_t n_value)
        : n(n_value) {
        if (n == 0) {
            throw std::invalid_argument("N parameter must be greater than 0.");
        }
    }

    std::string to_string() const {
        return "EAX_" + std::to_string(n) + "_AB";
    }

    std::size_t get_n() const {
        return n;
    }

protected:
    std::size_t n;
    static constexpr std::size_t parse_n(const std::string& str) {
        std::string start_str = "EAX_";
        std::string end_str = "_AB";
        if (!str.starts_with(start_str) || !str.ends_with(end_str)) {
            throw std::invalid_argument("Invalid EAX_n_AB format.");
        }
        std::string n_str = str.substr(start_str.size(), str.size() - (start_str.size() + end_str.size()));
        try {
            size_t n_value = std::stoul(n_str);
            return n_value;
        } catch (...) {
            throw std::invalid_argument("Invalid N parameter in EAX_n_AB.");
        }
    }
private:
};

template <size_t N>
class fixed_N_AB_tag : public EAX_n_AB_tag {
public:
    static bool match_string(const std::string& str) {
        return str == "EAX_" + std::to_string(N) + "_AB";
    }

    fixed_N_AB_tag(const std::string& str)
        : EAX_n_AB_tag(N) {
        if (parse_n(str) != N) {
            throw std::invalid_argument("N parameter does not match the fixed value.");
        }
    }
private:
};

using EAX_1_AB_tag = fixed_N_AB_tag<1>;
using EAX_5_AB_tag = fixed_N_AB_tag<5>;

}