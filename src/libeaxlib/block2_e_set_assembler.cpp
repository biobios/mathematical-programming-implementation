#include "block2_e_set_assembler.hpp"

namespace {
void set_initial_e_set(std::vector<size_t>& e_set,
                        size_t center_ab_cycle_index,
                        std::mt19937& rng,
                        size_t cycle_count,
                        std::vector<std::vector<size_t>> const& shared_vertex_count,
                        std::vector<size_t> const& AB_cycle_size) {
    using namespace std;

    uniform_int_distribution<size_t> dist01(0, 1);

    e_set.clear();
    e_set.reserve(cycle_count);
    e_set.push_back(center_ab_cycle_index);
    for (size_t i = 0; i < cycle_count; ++i) {
        if (shared_vertex_count[center_ab_cycle_index][i] > 0 &&
            AB_cycle_size[i] < AB_cycle_size[center_ab_cycle_index]) {
            if (dist01(rng) == 0) {
                e_set.push_back(i);
            }
        }
    }
}
}

namespace eax {

mpi::ObjectPool<std::vector<size_t>>::pooled_unique_ptr Block2ESetAssembler::operator()(size_t center_ab_cycle_index,
                                                                        std::mt19937& rng) {
    using namespace std;

    vector<size_t> const& AB_cycle_size = *AB_cycle_size_ptr;
    vector<size_t> const& c_vertex_count = *c_vertex_count_ptr;
    vector<vector<size_t>> const& shared_vertex_count = *shared_vertex_count_ptr;
    
    auto best_e_set_ptr = any_size_vector_pool->acquire_unique();
    vector<size_t>& best_e_set = *best_e_set_ptr;
    set_initial_e_set(best_e_set, center_ab_cycle_index, rng, cycle_count, shared_vertex_count, AB_cycle_size);

    auto shared_vertex_count_with_e_set_ptr = any_size_vector_pool->acquire_unique();
    vector<size_t>& shared_vertex_count_with_e_set = *shared_vertex_count_with_e_set_ptr;
    auto included_in_e_set_ptr = any_size_vector_pool->acquire_unique();
    vector<size_t>& included_in_e_set = *included_in_e_set_ptr;
    auto tabu_list_ptr = any_size_vector_pool->acquire_unique();
    vector<size_t>& tabu_list = *tabu_list_ptr;

    shared_vertex_count_with_e_set.resize(cycle_count, 0);
    included_in_e_set.resize(cycle_count, false);
    tabu_list.resize(cycle_count, 0);

    size_t current_num_c = 0;
    // ABサイクルを追加する関数
    auto add_cycle = [&c_vertex_count, &shared_vertex_count,
                        &shared_vertex_count_with_e_set, &included_in_e_set,
                        &current_num_c, this](size_t cycle_index){
        current_num_c += c_vertex_count[cycle_index] - 2 * shared_vertex_count_with_e_set[cycle_index];
        included_in_e_set[cycle_index] = true;

        for (size_t i = 0; i < cycle_count; ++i) {
            shared_vertex_count_with_e_set[i] += shared_vertex_count[cycle_index][i];
        }
    };

    // ABサイクルを削除する関数
    auto remove_cycle = [&c_vertex_count, &shared_vertex_count,
                        &shared_vertex_count_with_e_set, &included_in_e_set,
                        &current_num_c, this](size_t cycle_index) {
        current_num_c -= c_vertex_count[cycle_index] - 2 * shared_vertex_count_with_e_set[cycle_index];
        included_in_e_set[cycle_index] = false;

        for (size_t i = 0; i < cycle_count; ++i) {
            shared_vertex_count_with_e_set[i] -= shared_vertex_count[cycle_index][i];
        }
    };
    
    for (auto cycle_index : best_e_set) {
        add_cycle(cycle_index);
    }
    size_t best_num_c = current_num_c;
    
    uniform_int_distribution<size_t> tabu_dist(1, 10);
    
    size_t iteration = 0;
    size_t last_best_update_iteration = 0;
    while (true) {
        ++iteration;
        
        size_t min_num_c = numeric_limits<size_t>::max();
        size_t selected_cycle_index = 0;
        bool add = false;
        bool found = false;
        for (size_t i = 0; i < cycle_count; ++i) {
            bool is_tabu = tabu_list[i] >= iteration;
            if (!included_in_e_set[i] && shared_vertex_count_with_e_set[i] > 0) {
                size_t num_c = current_num_c + c_vertex_count[i] - 2 * shared_vertex_count_with_e_set[i];

                if ((num_c < best_num_c || !is_tabu) && num_c < min_num_c) {
                    min_num_c = num_c;
                    selected_cycle_index = i;
                    add = true;
                    found = true;
                }
            } else if (included_in_e_set[i] && i != center_ab_cycle_index) {
                size_t num_c = current_num_c - c_vertex_count[i] + 2 * shared_vertex_count_with_e_set[i];
                
                if ((num_c < best_num_c || !is_tabu) && num_c < min_num_c) {
                    min_num_c = num_c;
                    selected_cycle_index = i;
                    add = false;
                    found = true;
                }
            }
            
        }

        if (found) {
            if (add) {
                add_cycle(selected_cycle_index);
            } else {
                remove_cycle(selected_cycle_index);
            }
            
            tabu_list[selected_cycle_index] = iteration + tabu_dist(rng);
            
            if (current_num_c < best_num_c) { // 最良解が更新された
                best_num_c = current_num_c;
                last_best_update_iteration = iteration;
                best_e_set.clear();
                for (size_t i = 0; i < cycle_count; ++i) {
                    if (included_in_e_set[i]) {
                        best_e_set.push_back(i);
                    }
                }
            }
        }
        
        if (iteration - last_best_update_iteration >= 20) {
            break;
        }
    }
    
    return best_e_set_ptr;
}
}