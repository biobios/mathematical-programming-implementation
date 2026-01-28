#pragma once

#include <random>
#include <cstddef>

#include "object_pool.hpp"

#include "tsp_loader.hpp"
#include "object_pools.hpp"
#include "eax_normal.hpp"


namespace eax {
class uniform_e_set_assembler {
public:
    uniform_e_set_assembler(size_t ab_cycle_count, double target_size_ratio, mpi::ObjectPool<std::vector<size_t>>&& any_size_vector_pool)
        : ab_cycle_count(ab_cycle_count), target_size_ratio(target_size_ratio), any_size_vector_pool(std::move(any_size_vector_pool)) {}

    bool has_next() const {
        return ab_cycle_count > 0;
    }

    mpi::pooled_unique_ptr<std::vector<size_t>> next(std::mt19937& rng) {
        auto selected_AB_cycles_indices_ptr = any_size_vector_pool.acquire_unique();
        auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
        selected_AB_cycles_indices.clear();

        selected_AB_cycles_indices.resize(ab_cycle_count);
        std::iota(selected_AB_cycles_indices.begin(), selected_AB_cycles_indices.end(), 0);
        std::shuffle(selected_AB_cycles_indices.begin(), selected_AB_cycles_indices.end(), rng);

        size_t target_size = ab_cycle_count * target_size_ratio;
        target_size = std::clamp(target_size, static_cast<size_t>(1), ab_cycle_count);
        std::uniform_int_distribution<size_t> dist_size(1, target_size);
        
        selected_AB_cycles_indices.resize(dist_size(rng));

        return selected_AB_cycles_indices_ptr;
    }
private:
    size_t ab_cycle_count;
    double target_size_ratio;
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
};

class uniform_e_set_assembler_builder {
public:
    uniform_e_set_assembler_builder(ObjectPools& object_pools) :
        any_size_vector_pool(object_pools.any_size_vector_pool.share()) {}
    
    uniform_e_set_assembler build(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles, const auto&, const auto&, size_t, const tsp::TSP&, std::mt19937&, double target_size_ratio = 1.0) {
        return uniform_e_set_assembler(AB_cycles.size(), target_size_ratio, any_size_vector_pool.share());
    }

    static size_t calc_AB_cycle_need(const auto&, const auto&, size_t, const tsp::TSP&, std::mt19937&, [[maybe_unused]]double target_size_ratio = 1.0) {
        return std::numeric_limits<size_t>::max();
    }

    template <typename CompletenessCategory>
    static constexpr bool is_available(CompletenessCategory) {
        // 完全なABサイクル集合を必要としないため、常に真を返す
        return true;
    }
private:
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
};

using EAX_UNIFORM = EAX_normal<uniform_e_set_assembler_builder>;

class EAX_UNIFORM_tag {
public:
    static bool match_string(const std::string& str) {
        if (str == "EAX_UNIFORM") {
            return true;
        }

        std::string start_str = "EAX_";
        std::string end_str = "_UNIFORM";

        if (!str.starts_with(start_str) || !str.ends_with(end_str)) {
            return false;
        }

        std::string ratio_str = str.substr(start_str.size(), str.size() - (start_str.size() + end_str.size()));

        if (ratio_str == "half") {
            return true;
        }

        try {
            double ratio_value = std::stod(ratio_str);
            return ratio_value > 0.0 && ratio_value <= 1.0;
        } catch (...) {
            return false;
        }
    }

    EAX_UNIFORM_tag() : ratio(1.0) {}
    EAX_UNIFORM_tag(const std::string& str)
        : ratio(parse_ratio(str)) {}
    
    std::string to_string() const {
        if (ratio == 1.0) {
            return "EAX_UNIFORM";
        } else if (ratio == 0.5) {
            return "EAX_half_UNIFORM";
        } else {
            return "EAX_" + std::to_string(ratio) + "_UNIFORM";
        }
    }

    double get_ratio() const {
        return ratio;
    }
protected:
    double ratio;
    static constexpr double parse_ratio(const std::string& str) {
        if (str == "EAX_UNIFORM") {
            return 1.0;
        }

        std::string start_str = "EAX_";
        std::string end_str = "_UNIFORM";

        if (!str.starts_with(start_str) || !str.ends_with(end_str)) {
            throw std::invalid_argument("Invalid EAX_UNIFORM format.");
        }

        std::string ratio_str = str.substr(start_str.size(), str.size() - (start_str.size() + end_str.size()));

        if (ratio_str == "half") {
            return 0.5;
        }

        try {
            double ratio_value = std::stod(ratio_str);
            if (ratio_value <= 0.0 || ratio_value > 1.0) {
                throw std::invalid_argument("Ratio must be in the range (0.0, 1.0].");
            }
            return ratio_value;
        } catch (...) {
            throw std::invalid_argument("Invalid ratio parameter in EAX_UNIFORM.");
        }
    }
private:
};

template <double RATIO>
class fixed_UNIFORM_tag : public EAX_UNIFORM_tag {
public:
    static bool match_string(const std::string& str) {
        if (!EAX_UNIFORM_tag::match_string(str)) {
            return false;
        }

        double parsed_ratio = EAX_UNIFORM_tag::parse_ratio(str);
        return parsed_ratio == RATIO;
    }

    fixed_UNIFORM_tag(const std::string& str)
        : EAX_UNIFORM_tag(str) {
        if (ratio != RATIO) {
            throw std::invalid_argument("Ratio does not match fixed ratio.");
        }
    }
};

using EAX_full_UNIFORM_tag = fixed_UNIFORM_tag<1.0>;
using EAX_half_UNIFORM_tag = fixed_UNIFORM_tag<0.5>;
}