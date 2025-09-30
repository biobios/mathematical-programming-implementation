#include "eax_tabu.hpp"
namespace eax {
// タブーエッジを含むABサイクルを削除する
void EAX_tabu::remove_tabu_AB_cycles(std::vector<mpi::pooled_unique_ptr<ab_cycle_t>>& AB_cycles,
                            const std::vector<std::pair<size_t, size_t>>& tabu_edges) {
    // 一つの都市は最大2つのABサイクルに含まれるので、vector_of_tsp_sizeを2つ用意する
    auto in_cycle1_ptr = vector_of_tsp_size_pool.acquire_unique();
    auto in_cycle2_ptr = vector_of_tsp_size_pool.acquire_unique();
    std::vector<size_t>& in_cycle1 = *in_cycle1_ptr;
    std::vector<size_t>& in_cycle2 = *in_cycle2_ptr;
    const size_t null_cycle = AB_cycles.size();
    in_cycle1.assign(in_cycle1.size(), null_cycle);
    in_cycle2.assign(in_cycle2.size(), null_cycle);

    for (size_t i = 0; i < AB_cycles.size(); ++i) {
        const ab_cycle_t& cycle = *AB_cycles[i];
        for (size_t city : cycle) {
            if (in_cycle1[city] == null_cycle) {
                in_cycle1[city] = i;
            } else {
                in_cycle2[city] = i;
            }
        }
    }
    
    // タブーエッジを含むABサイクルを削除する
    for (const auto& [v1, v2] : tabu_edges) {
        size_t cycle_v1_1 = in_cycle1[v1];
        size_t cycle_v1_2 = in_cycle2[v1];
        size_t cycle_v2_1 = in_cycle1[v2];
        size_t cycle_v2_2 = in_cycle2[v2];
        if ((cycle_v1_1 == cycle_v2_1) || (cycle_v1_1 == cycle_v2_2)) {
            if (cycle_v1_1 != null_cycle) {
                AB_cycles[cycle_v1_1].reset();
            }
        }
        if ((cycle_v1_2 == cycle_v2_1) || (cycle_v1_2 == cycle_v2_2)) {
            if (cycle_v1_2 != null_cycle) {
                AB_cycles[cycle_v1_2].reset();
            }
        }
    }
    
    // nullptrになった要素を削除する
    for (size_t i = AB_cycles.size(); i > 0; --i) {
        if (AB_cycles[i - 1] == nullptr) {
            if (i - 1 != AB_cycles.size() - 1) {
                std::swap(AB_cycles[i - 1], AB_cycles.back());
            }
            AB_cycles.pop_back();
        }
    }
}

std::vector<CrossoverDelta> EAX_tabu::generate_children_via_EAX_UNIFORM(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>> &AB_cycles, size_t children_size, IntermediateIndividual &working_individual, const tsp::TSP &tsp, std::mt19937 &rng)
{
    if (AB_cycles.empty()) {
        return {};
    }

    auto AB_cycle_indices_ptr = any_size_vector_pool.acquire_unique();
    auto& AB_cycle_indices = *AB_cycle_indices_ptr;
    AB_cycle_indices.resize(AB_cycles.size());
    iota(AB_cycle_indices.begin(), AB_cycle_indices.end(), 0);
    
    std::uniform_int_distribution<int> cycle_range_dist(1, AB_cycles.size());
    std::vector<CrossoverDelta> children;
    
    for (size_t child_index = 0; child_index < children_size; ++child_index) {
        shuffle(AB_cycle_indices.begin(), AB_cycle_indices.end(), rng);

        auto selected_AB_cycles_indices_ptr = any_size_vector_pool.acquire_unique();
        auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
        selected_AB_cycles_indices.clear();
        size_t cycle_count = cycle_range_dist(rng);
        selected_AB_cycles_indices.insert(selected_AB_cycles_indices.end(),
                                            AB_cycle_indices.begin(),
                                            AB_cycle_indices.begin() + cycle_count);
        
        auto selected_AB_cycles_view = selected_AB_cycles_indices | std::views::transform([&AB_cycles](size_t index) -> const ab_cycle_t& {
            return *AB_cycles[index];
        });
        
        working_individual.apply_AB_cycles(selected_AB_cycles_view);

        subtour_merger(working_individual, tsp, selected_AB_cycles_view);
        
        children.emplace_back(working_individual.get_delta_and_revert());
    }
    
    if (children.empty()) {
        children.emplace_back(working_individual.get_delta_and_revert());
    }

    return children;
}

std::vector<CrossoverDelta> EAX_tabu::generate_children_via_EAX_half_UNIFORM(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>> &AB_cycles, size_t children_size, IntermediateIndividual &working_individual, const tsp::TSP &tsp, std::mt19937 &rng)
{
    if (AB_cycles.empty()) {
        return {};
    }
    
    auto AB_cycle_indices_ptr = any_size_vector_pool.acquire_unique();
    auto& AB_cycle_indices = *AB_cycle_indices_ptr;
    AB_cycle_indices.resize(AB_cycles.size());
    iota(AB_cycle_indices.begin(), AB_cycle_indices.end(), 0);
    
    size_t half_size = (AB_cycles.size() + 1) / 2;
    std::uniform_int_distribution<int> cycle_range_dist(1, half_size);
    std::vector<CrossoverDelta> children;
    
    for (size_t child_index = 0; child_index < children_size; ++child_index) {
        shuffle(AB_cycle_indices.begin(), AB_cycle_indices.end(), rng);

        auto selected_AB_cycles_indices_ptr = any_size_vector_pool.acquire_unique();
        auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
        selected_AB_cycles_indices.clear();
        size_t cycle_count = cycle_range_dist(rng);
        selected_AB_cycles_indices.insert(selected_AB_cycles_indices.end(),
                                            AB_cycle_indices.begin(),
                                            AB_cycle_indices.begin() + cycle_count);
        
        auto selected_AB_cycles_view = selected_AB_cycles_indices | std::views::transform([&AB_cycles](size_t index) -> const ab_cycle_t& {
            return *AB_cycles[index];
        });
        
        working_individual.apply_AB_cycles(selected_AB_cycles_view);

        subtour_merger(working_individual, tsp, selected_AB_cycles_view);
        
        children.emplace_back(working_individual.get_delta_and_revert());
    }
    
    if (children.empty()) {
        children.emplace_back(working_individual.get_delta_and_revert());
    }

    return children;
}

std::vector<CrossoverDelta> EAX_tabu::generate_children_via_EAX_1AB(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>> &AB_cycles, size_t children_size, IntermediateIndividual &working_individual, const tsp::TSP &tsp, std::mt19937 &rng)
{
    using namespace std;
    vector<CrossoverDelta> children;
    children_size = min(children_size, AB_cycles.size());

    auto AB_cycle_indices_ptr = any_size_vector_pool.acquire_unique();
    auto& AB_cycle_indices = *AB_cycle_indices_ptr;
    AB_cycle_indices.resize(AB_cycles.size());
    iota(AB_cycle_indices.begin(), AB_cycle_indices.end(), 0);
    shuffle(AB_cycle_indices.begin(), AB_cycle_indices.end(), rng);


    for (size_t child_index = 0; child_index < children_size; ++child_index) {

        // 緩和個体を作成
        auto selected_AB_cycles_indices_ptr = any_size_vector_pool.acquire_unique();
        auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
        selected_AB_cycles_indices.clear();

        selected_AB_cycles_indices.push_back(AB_cycle_indices[child_index]);

        auto selected_AB_cycles_view = selected_AB_cycles_indices | views::transform([&AB_cycles](size_t index) -> const ab_cycle_t& {
            return *AB_cycles[index];
        });

        working_individual.apply_AB_cycles(selected_AB_cycles_view);

        subtour_merger(working_individual, tsp, selected_AB_cycles_view);

        children.emplace_back(working_individual.get_delta_and_revert());

    }

    if (children.empty()) {
        children.emplace_back(working_individual.get_delta_and_revert());
    }
    return children;
}

std::vector<CrossoverDelta> EAX_tabu::generate_children_via_EAX_Rand(const std::vector<mpi::pooled_unique_ptr<ab_cycle_t>> &AB_cycles, size_t children_size, IntermediateIndividual &working_individual, const tsp::TSP &tsp, std::mt19937 &rng)
{
    using namespace std;
    vector<CrossoverDelta> children;
    
    if (AB_cycles.empty()) {
        return children;
    }
    
    std::bernoulli_distribution bernoulli(0.5);

    for (size_t child_index = 0; child_index < children_size; ++child_index) {
        auto selected_AB_cycles_indices_ptr = any_size_vector_pool.acquire_unique();
        auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
        selected_AB_cycles_indices.clear();
        
        for (size_t i = 0; i < AB_cycles.size(); ++i) {
            if (bernoulli(rng)) {
                selected_AB_cycles_indices.push_back(i);
            }
        }
        
        if (selected_AB_cycles_indices.empty()) {
            children.emplace_back(working_individual.get_delta_and_revert());
            continue;
        }
        
        auto selected_AB_cycles_view = selected_AB_cycles_indices | views::transform([&AB_cycles](size_t index) -> const ab_cycle_t& {
            return *AB_cycles[index];
        });

        working_individual.apply_AB_cycles(selected_AB_cycles_view);
        subtour_merger(working_individual, tsp, selected_AB_cycles_view);
        children.emplace_back(working_individual.get_delta_and_revert());
    }
    
    if (children.empty()) {
        children.emplace_back(working_individual.get_delta_and_revert());
    }

    return children;
}
}