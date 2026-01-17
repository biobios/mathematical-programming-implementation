#include <bit>

#include "crossover_delta.hpp"

namespace eax {
int64_t CrossoverDelta::get_delta_distance() const {
    return this->delta_distance;
}

uint64_t CrossoverDelta::compute_delta_checksum(const std::vector<Modification> &modifications)
{
    if (modifications.empty()) {
        return 0;
    }

    auto hash_mod = [](const Modification& mod) {
        constexpr uint64_t MIX_A = 0x9e3779b97f4a7c15;
        constexpr uint64_t MIX_B = 0xbf58476d1ce4e5b9;
        constexpr uint64_t MIX_C = 0x94d049bb133111eb;

        return (mod.edge1.first * MIX_A) ^ (mod.edge1.second * MIX_B) ^ (mod.new_v2 * MIX_C);
    };

    const auto& first = modifications.front();
    const auto& mid = modifications[modifications.size() / 2];
    const auto& last = modifications.back();

    uint64_t h_first = hash_mod(first);
    uint64_t h_mid = hash_mod(mid);
    uint64_t h_last = hash_mod(last);
    
    return h_first ^ std::rotl(h_mid, 19) ^ std::rotl(h_last, 41);
}
}