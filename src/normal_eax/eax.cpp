#include "eax.hpp"

#include <vector>
#include <random>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <array>
#include <tuple>
#include <algorithm>
#include <numeric>
#include <utility>
#include <chrono>
#include <iostream>
#include <map>
#include <ranges>

#include "limited_range_integer_set.hpp"
#include "environment.hpp"
#include "ab_cycle_finder.hpp"

namespace {
using namespace eax;
using ABCycle_ptr = decltype(Environment::object_pools.any_size_vector_pool)::pooled_unique_ptr;

std::map<std::string, double> times;
std::map<std::string, std::chrono::high_resolution_clock::time_point> start_times;

// constexpr void start_timer(const std::string& name) {
//     start_times[name] = std::chrono::high_resolution_clock::now();
// }

// constexpr void end_timer(const std::string& name) {
//     auto end_time = std::chrono::high_resolution_clock::now();
//     times[name] += std::chrono::duration<double>(end_time - start_times[name]).count();
// }

constexpr void start_timer(const std::string&) {}

constexpr void end_timer(const std::string&) {}

void merge_sub_tours(const std::vector<std::vector<int64_t>>& adjacency_matrix,
            IntermediateIndividual& child,
            const std::vector<size_t>& path,
            const std::vector<size_t>& pos,
            const std::vector<std::vector<std::pair<int64_t, size_t>>>& NN_list,
            eax::Environment& env)
{
    using namespace std;
    using distance_type = std::remove_cvref_t<decltype(adjacency_matrix)>::value_type::value_type;
    using edge = pair<size_t, size_t>;
    // vector<size_t> elem_of_min_sub_tour;
    // vector<bool> in_min_sub_tour(path.size(), false);
    auto elem_of_min_sub_tour_ptr = env.object_pools.any_size_vector_pool.acquire_unique();
    auto in_min_sub_tour_ptr = env.object_pools.in_min_sub_tour_pool.acquire_unique();
    auto& elem_of_min_sub_tour = *elem_of_min_sub_tour_ptr;
    auto& in_min_sub_tour = *in_min_sub_tour_ptr;

    while (child.sub_tour_count() > 1) {
        start_timer("find_min_size_sub_tour");
        auto [min_sub_tour_id, min_sub_tour_size] = child.find_min_size_sub_tour();
        size_t start_city = path[child.get_city_pos_of_sub_tour(min_sub_tour_id)];

        elem_of_min_sub_tour.clear();
        elem_of_min_sub_tour.reserve(min_sub_tour_size + 2);
        size_t prev_city = start_city;
        size_t current_city = start_city;
        do {
            elem_of_min_sub_tour.push_back(current_city);
            in_min_sub_tour[current_city] = true;
            size_t next_city = child[current_city][0];
            if (next_city == prev_city) {
                next_city = child[current_city][1];
            }
            prev_city = current_city;
            current_city = next_city;
        } while (current_city != start_city);
        
        elem_of_min_sub_tour.push_back(elem_of_min_sub_tour[0]);
        elem_of_min_sub_tour.push_back(elem_of_min_sub_tour[1]);
        
        end_timer("find_min_size_sub_tour");
        start_timer("find_edges_to_swap");
        
        size_t search_range = 10;
        size_t start = 0;
        edge e1 = {0, 0};
        edge e2 = {0, 0};
        distance_type min_cost = std::numeric_limits<distance_type>::max();
        while (e1.first == 0 && e2.first == 0) {
            for (size_t i = 1; i <= min_sub_tour_size; ++i) {
                size_t current_city = elem_of_min_sub_tour[i];
                size_t limit = std::min(start + search_range, NN_list[current_city].size());
                for (size_t j = start; j < limit; ++j) {
                    size_t neighbor_city = NN_list[current_city][j].second;
                    if (in_min_sub_tour[neighbor_city])
                        continue;

                    for (size_t k = 0; k < 2; ++k) {
                        size_t connected_to_current_city = elem_of_min_sub_tour[i - 1 + 2 * k];
                        for (size_t l = 0; l < 2; ++l) {
                            size_t connected_to_neighbor_city = child[neighbor_city][l];

                            distance_type cost = - adjacency_matrix[current_city][connected_to_current_city] - adjacency_matrix[neighbor_city][connected_to_neighbor_city]
                                                 + adjacency_matrix[current_city][neighbor_city] + adjacency_matrix[connected_to_current_city][connected_to_neighbor_city];
                            
                            if (cost < min_cost) {
                                min_cost = cost;
                                e1 = {current_city, connected_to_current_city};
                                e2 = {neighbor_city, connected_to_neighbor_city};
                            }
                            
                            cost = - adjacency_matrix[current_city][connected_to_current_city] - adjacency_matrix[neighbor_city][connected_to_neighbor_city]
                                   + adjacency_matrix[current_city][connected_to_neighbor_city] + adjacency_matrix[connected_to_current_city][neighbor_city];

                            if (cost < min_cost) {
                                min_cost = cost;
                                e1 = {current_city, connected_to_current_city};
                                e2 = {connected_to_neighbor_city, neighbor_city};
                            }
                        }
                    }
                }
            }
            
            start += search_range;
            search_range *= 2;
        }
        
        end_timer("find_edges_to_swap");
        start_timer("swap_edges_and_merge_sub_tours");
        
        child.swap_edges(e1, e2);
        size_t connected_to_min_sub_tour = child.find_sub_tour_containing(pos[e2.first]);
        child.merge_sub_tour(min_sub_tour_id, connected_to_min_sub_tour);
        
        for (size_t city : elem_of_min_sub_tour) {
            in_min_sub_tour[city] = false;
        }
        
        end_timer("swap_edges_and_merge_sub_tours");
    }
}

} // namespace

namespace eax {
eax::Child IntermediateIndividual::convert_to_child_and_revert() {
    revert();
    eax::Child child(std::move(modifications));
    modifications.clear();
    return child;
}

void IntermediateIndividual::assign(const eax::Individual& parent) {
    working_individual = parent;
    reset();
}

void IntermediateIndividual::swap_edges(std::pair<size_t, size_t> edge1, std::pair<size_t, size_t> edge2) {
    auto [v1, v2] = edge1;
    auto [u1, u2] = edge2;
    // v1 -> v2 => v1 -> u1
    change_connection(v1, v2, u1);
    // v2 -> v1 => v2 -> u2
    change_connection(v2, v1, u2);
    // u1 -> u2 => u1 -> v1
    change_connection(u1, u2, v1);
    // u2 -> u1 => u2 -> v2
    change_connection(u2, u1, v2);
}

void IntermediateIndividual::change_connection(size_t v1, size_t v2, size_t new_v2) {
    eax::Child::Modification modification{
        {v1, v2},
        new_v2
    };
    modifications.push_back(modification);
    
    if (working_individual[v1][0] == v2) {
        working_individual[v1][0] = new_v2;
    } else {
        working_individual[v1][1] = new_v2;
    }
}

size_t IntermediateIndividual::sub_tour_count() const {
    return sub_tour_sizes.size();
}

std::pair<size_t, size_t> IntermediateIndividual::find_min_size_sub_tour() const {
    size_t min_size = std::numeric_limits<size_t>::max();
    size_t min_size_sub_tour_id = 0;
    for (size_t i = 0; i < sub_tour_sizes.size(); ++i) {
        if (sub_tour_sizes[i] < min_size) {
            min_size = sub_tour_sizes[i];
            min_size_sub_tour_id = i;
        }
    }
    return {min_size_sub_tour_id, min_size};
}

size_t IntermediateIndividual::find_sub_tour_containing(size_t pos) const {
    for (const auto& segment : segments) {
        if (segment.beginning_pos <= pos && pos <= segment.end_pos) {
            return segment.sub_tour_ID;
        }
    }
    throw std::runtime_error("No sub-tour found containing the position");
}

size_t IntermediateIndividual::get_city_pos_of_sub_tour(size_t sub_tour_id) const {
    for (const auto& segment : segments) {
        if (segment.sub_tour_ID == sub_tour_id) {
            return segment.beginning_pos;
        }
    }
    throw std::runtime_error("No sub-tour found with the given ID");
}

void IntermediateIndividual::merge_sub_tour(size_t sub_tour_id1, size_t sub_tour_id2) {
    for (auto& segment : segments) {
        if (segment.sub_tour_ID == sub_tour_id2) {
            segment.sub_tour_ID = sub_tour_id1;
        }
    }
    
    sub_tour_sizes[sub_tour_id1] += sub_tour_sizes[sub_tour_id2];
    
    size_t last_index = sub_tour_sizes.size() - 1;
    if (sub_tour_id2 < last_index) {
        for (auto& segment : segments) {
            if (segment.sub_tour_ID == last_index) {
                segment.sub_tour_ID = sub_tour_id2;
            }
        }
        
        sub_tour_sizes[sub_tour_id2] = sub_tour_sizes[last_index];
    }
    
    sub_tour_sizes.pop_back();
}

const std::array<size_t, 2>& IntermediateIndividual::operator[](size_t index) {
    return working_individual[index];
}

const std::array<size_t, 2>& IntermediateIndividual::operator[](size_t index) const {
    return working_individual[index];
}

void IntermediateIndividual::revert() {
    for (auto it = modifications.crbegin(); it != modifications.crend(); ++it) {
        undo(*it);
    }
}

void IntermediateIndividual::reset() {
    modifications.clear();
    segments.clear();
    sub_tour_sizes.clear();
}

void IntermediateIndividual::undo(const eax::Child::Modification& modification) {
    auto [v1, v2] = modification.edge1;
    size_t new_v2 = modification.new_v2;
    if (working_individual[v1][0] == new_v2) {
        working_individual[v1][0] = v2;
    } else {
        working_individual[v1][1] = v2;
    }
}

template <std::ranges::range ABCycles>
    requires std::convertible_to<std::ranges::range_value_t<ABCycles>, const ABCycle&>
void IntermediateIndividual::apply_AB_cycles(const ABCycles& AB_cycles,
                        const std::vector<size_t>& pos,
                        Environment& env) {
    using namespace std;
    const size_t city_count = working_individual.size();
    start_timer("apply_AB_cycles");
    // tuple <size_t, size_t, size_t> (a, b, c)
    // a: beginning or end of segment
    // b: adjacent to a
    // c: adjacent to (a - 1)
    // vector<tuple<size_t, size_t, size_t>> cut_positions;
    auto cut_positions_ptr = env.object_pools.cut_positions_pool.acquire_unique();
    auto& cut_positions = *cut_positions_ptr;
    cut_positions.clear();

    bool cut_between_first_and_last = false;
    auto cut = [&cut_positions, &cut_between_first_and_last, this, &pos, city_count](size_t b1, size_t ba, size_t ab, size_t b2) {
        change_connection(ba, ab, b1);
        change_connection(ab, ba, b2);
        auto pos_b1 = pos[b1];
        auto pos_ba = pos[ba];
        auto pos_ab = pos[ab];
        auto pos_b2 = pos[b2];
        
        if (pos_ba == 0 && pos_ab == city_count - 1) {
            cut_positions.emplace_back(pos_ba, pos_b1, pos_b2);
            cut_positions.emplace_back(city_count, pos_b1, pos_b2);
            cut_between_first_and_last = true;
        } else if (pos_ba == city_count - 1 && pos_ab == 0) {
            cut_positions.emplace_back(pos_ab, pos_b2, pos_b1);
            cut_positions.emplace_back(city_count, pos_b2, pos_b1);
            cut_between_first_and_last = true;
        } else if (pos_ba < pos_ab) {
            cut_positions.emplace_back(pos_ab, pos_b2, pos_b1);
        } else {
            cut_positions.emplace_back(pos_ba, pos_b1, pos_b2);
        }
    };
    for (const ABCycle& cycle : AB_cycles) {
        for (size_t i = 2; i < cycle.size() - 2; i += 2) {
            cut(cycle[i - 1], cycle[i], cycle[i + 1], cycle[i + 2]);
        }
        // i = 0
        {
            cut(cycle[cycle.size() - 1], cycle[0], cycle[1], cycle[2]);
        }
        // i = cycle.size() - 2
        {
            cut(cycle[cycle.size() - 3], cycle[cycle.size() - 2], cycle[cycle.size() - 1], cycle[0]);
        }
    }
    
    if (!cut_between_first_and_last) {
        cut_positions.emplace_back(0, city_count - 1, 0);
        cut_positions.emplace_back(city_count, city_count - 1, 0);
    }
    
    end_timer("apply_AB_cycles");
    start_timer("construct_segments");

    std::sort(cut_positions.begin(), cut_positions.end());

    auto pos_to_segment_id_ptr = env.object_pools.vector_of_tsp_size_pool.acquire_unique();
    auto& pos_to_segment_id = *pos_to_segment_id_ptr;

    segments.reserve(cut_positions.size());
    auto add_segment = [this, &cut_positions, &pos_to_segment_id](size_t begin_i, size_t end_i) {
        auto [beginning_pos, beginning_adjacent_pos, beginning_minus_one_adjacent_pos] = cut_positions[begin_i];
        auto [end_pos_plus_one, end_pos_plus_one_adjacent_pos, end_adjacent_pos] = cut_positions[end_i];
        Segment segment {
            .ID = segments.size(),
            .beginning_pos = beginning_pos,
            .end_pos = end_pos_plus_one - 1,
            .beginning_adjacent_pos = beginning_adjacent_pos,
            .end_adjacent_pos = end_adjacent_pos,
            .sub_tour_ID = std::numeric_limits<size_t>::max()
        };
        pos_to_segment_id[beginning_pos] = segment.ID;
        pos_to_segment_id[end_pos_plus_one - 1] = segment.ID;
        segments.push_back(segment);
    };

    for (size_t i = 0; i < cut_positions.size() - 1; ++i) {
        add_segment(i, i + 1);
    }
    
    // sub_tour_ID を設定
    size_t sub_tour_id = 0;
    while (true) {
        size_t found_segment = std::numeric_limits<size_t>::max();
        for (size_t i = 0; i < segments.size(); ++i) {
            if (segments[i].sub_tour_ID == std::numeric_limits<size_t>::max()) {
                found_segment = i;
                break;
            }
        }
        if (found_segment == std::numeric_limits<size_t>::max()) {
            break;
        }

        size_t current_segment = found_segment;
        while(true) {
            segments[current_segment].sub_tour_ID = sub_tour_id;
            size_t next_segment = pos_to_segment_id[segments[current_segment].beginning_adjacent_pos];

            if (segments[next_segment].sub_tour_ID != std::numeric_limits<size_t>::max()) {
                next_segment = pos_to_segment_id[segments[current_segment].end_adjacent_pos];
                if (segments[next_segment].sub_tour_ID != std::numeric_limits<size_t>::max()) {
                    // 1周した
                    break;
                }
            }
            
            current_segment = next_segment;
        } 
        
        sub_tour_id++;
    }

    sub_tour_sizes.resize(sub_tour_id, 0);

    size_t forcused_segment_id = 0;
    for (const auto& segment : segments) {
        if (segment.sub_tour_ID == segments[forcused_segment_id].sub_tour_ID) {
            segments[forcused_segment_id].end_pos = segment.end_pos;
            segments[forcused_segment_id].end_adjacent_pos = segment.end_adjacent_pos;
            sub_tour_sizes[segment.sub_tour_ID] += segment.end_pos - segment.beginning_pos + 1;
        } else {
            ++forcused_segment_id;
            segments[forcused_segment_id] = segment;
            sub_tour_sizes[segment.sub_tour_ID] = segment.end_pos - segment.beginning_pos + 1;
        }
    }
    segments.resize(forcused_segment_id + 1);
    end_timer("construct_segments");
}
namespace {
    std::vector<Child> edge_assembly_crossover_N_AB(const Individual& parent1, const Individual& parent2, size_t children_size,
                                            eax::Environment& env, std::mt19937& rng) {
        auto& tsp = env.tsp;
        auto& adjacency_matrix = tsp.adjacency_matrix;
        auto& NN_list = tsp.NN_list;
        using namespace std;

        const size_t n = parent1.size();

        auto path_ptr = env.object_pools.vector_of_tsp_size_pool.acquire_unique();
        auto pos_ptr = env.object_pools.vector_of_tsp_size_pool.acquire_unique();
        vector<size_t>& path = *path_ptr;
        vector<size_t>& pos = *pos_ptr;
        for (size_t i = 0, prev = 0, current = 0; i < n; ++i) {
            path[i] = current;
            pos[current] = i;
            size_t next = parent1[current][0];
            if (next == prev) {
                next = parent1[current][1];
            }
            prev = current;
            current = next;
        }

        start_timer("find_AB_cycles");
        vector<ABCycle_ptr> AB_cycles = find_AB_cycles(env.N_parameter * children_size, parent1, parent2, rng, env.object_pools.any_size_vector_pool, env.object_pools.vector_of_tsp_size_pool, env.object_pools.doubly_linked_list_pool, env.object_pools.LRIS_pool);
        end_timer("find_AB_cycles");

        auto AB_cycle_indices_ptr = env.object_pools.any_size_vector_pool.acquire_unique();
        auto& AB_cycle_indices = *AB_cycle_indices_ptr;
        AB_cycle_indices.resize(AB_cycles.size());
        iota(AB_cycle_indices.begin(), AB_cycle_indices.end(), 0);
        shuffle(AB_cycle_indices.begin(), AB_cycle_indices.end(), rng);

        vector<Child> children;
        auto working_individual = env.object_pools.intermediate_individual_pool.acquire_unique();
        working_individual->assign(parent1);

        children_size = min(children_size, (AB_cycles.size() + env.N_parameter - 1) / env.N_parameter);

        for (size_t child_index = 0; child_index < children_size; ++child_index) {

            // 緩和個体を作成
            start_timer("create_relaxed_individual");

            auto selected_AB_cycles_indices_ptr = env.object_pools.any_size_vector_pool.acquire_unique();
            auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
            selected_AB_cycles_indices.clear();

            for (size_t i = 0; i < env.N_parameter && i < AB_cycles.size(); ++i) {
                size_t index = (child_index * env.N_parameter + i) % AB_cycles.size();
                selected_AB_cycles_indices.push_back(AB_cycle_indices[index]);
            }

            auto selected_AB_cycles_view = selected_AB_cycles_indices | views::transform([&AB_cycles](size_t index) -> const ABCycle& {
                return *AB_cycles[index];
            });

            working_individual->apply_AB_cycles(selected_AB_cycles_view, pos, env);

            end_timer("create_relaxed_individual");

            start_timer("merge_sub_tours");

            merge_sub_tours(adjacency_matrix, *working_individual, path, pos, NN_list, env);

            end_timer("merge_sub_tours");

            children.emplace_back(working_individual->convert_to_child_and_revert());

        }

        if (children.empty()) {
            children.emplace_back(working_individual->convert_to_child_and_revert());
        }
        return children;

    }
    
    std::vector<Child> edge_assembly_crossover_Rand(const Individual& parent1, const Individual& parent2, size_t children_size,
                                            eax::Environment& env, std::mt19937& rng) {
        auto& tsp = env.tsp;
        auto& adjacency_matrix = tsp.adjacency_matrix;
        auto& NN_list = tsp.NN_list;
        using namespace std;

        const size_t n = parent1.size();
                                                
        auto path_ptr = env.object_pools.vector_of_tsp_size_pool.acquire_unique();
        auto pos_ptr = env.object_pools.vector_of_tsp_size_pool.acquire_unique();
        vector<size_t>& path = *path_ptr;
        vector<size_t>& pos = *pos_ptr;
        for (size_t i = 0, prev = 0, current = 0; i < n; ++i) {
            path[i] = current;
            pos[current] = i;
            size_t next = parent1[current][0];
            if (next == prev) {
                next = parent1[current][1];
            }
            prev = current;
            current = next;
        }

        start_timer("find_AB_cycles");
        vector<ABCycle_ptr> AB_cycles = find_AB_cycles(numeric_limits<size_t>::max(), parent1, parent2, rng, env.object_pools.any_size_vector_pool, env.object_pools.vector_of_tsp_size_pool, env.object_pools.doubly_linked_list_pool, env.object_pools.LRIS_pool);
        end_timer("find_AB_cycles");

        vector<Child> children;
        auto working_individual = env.object_pools.intermediate_individual_pool.acquire_unique();
        working_individual->assign(parent1);

        if (env.eax_type == eax::EAXType::N_AB) {
            children_size = min(children_size, (AB_cycles.size() + env.N_parameter - 1) / env.N_parameter);
        }
        for (size_t child_index = 0; child_index < children_size; ++child_index) {

            // 緩和個体を作成
            start_timer("create_relaxed_individual");

            auto selected_AB_cycles_indices_ptr = env.object_pools.any_size_vector_pool.acquire_unique();
            auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
            selected_AB_cycles_indices.clear();

            uniform_int_distribution<size_t> dist_01(0, 1);
            for (size_t i = 0; i < AB_cycles.size(); ++i) {
                if (dist_01(rng) == 0) {
                    selected_AB_cycles_indices.push_back(i);
                }
            }

            auto selected_AB_cycles_view = selected_AB_cycles_indices | views::transform([&AB_cycles](size_t index) -> const ABCycle& {
                return *AB_cycles[index];
            });

            working_individual->apply_AB_cycles(selected_AB_cycles_view, pos, env);

            end_timer("create_relaxed_individual");

            start_timer("merge_sub_tours");

            merge_sub_tours(adjacency_matrix, *working_individual, path, pos, NN_list, env);

            end_timer("merge_sub_tours");

            children.emplace_back(working_individual->convert_to_child_and_revert());

        }
        if (children.empty()) {
            children.emplace_back(working_individual->convert_to_child_and_revert());
        }
        return children;
    }
    
    class Block2Strategy {
    public:
        Block2Strategy(const eax::Individual& parent1,
                        const eax::Individual& parent2,
                        const std::vector<ABCycle_ptr>& AB_cycles,
                        size_t city_count,
                        mpi::ObjectPool<std::vector<size_t>>& vector_of_tsp_size_pool,
                        mpi::ObjectPool<std::vector<size_t>>& any_size_vector_pool,
                        mpi::ObjectPool<std::vector<std::vector<size_t>>>& shared_vertex_count_pool)
                        : city_count(city_count),
                          cycle_count(AB_cycles.size()),
                          c_vertex_count_ptr(any_size_vector_pool.acquire_unique()),
                          shared_vertex_count_ptr(shared_vertex_count_pool.acquire_unique()) {
            using namespace std;
            auto belongs_to_AB_cycle1_ptr = vector_of_tsp_size_pool.acquire_unique();
            auto belongs_to_AB_cycle2_ptr = vector_of_tsp_size_pool.acquire_unique();
            vector<size_t>& belongs_to_AB_cycle1 = *belongs_to_AB_cycle1_ptr;
            vector<size_t>& belongs_to_AB_cycle2 = *belongs_to_AB_cycle2_ptr;
            
            const size_t NULL_CYCLE = std::numeric_limits<size_t>::max();
            for (size_t i = 0; i < city_count; ++i) {
                belongs_to_AB_cycle1[i] = NULL_CYCLE;
                belongs_to_AB_cycle2[i] = NULL_CYCLE;
            }
            
            // 各頂点が属するABサイクルを記録
            for (size_t i = 0; i < cycle_count; ++i) {
                const ABCycle& cycle = *AB_cycles[i];
                for (auto city : cycle) {
                    if (belongs_to_AB_cycle1[city] == NULL_CYCLE) {
                        belongs_to_AB_cycle1[city] = i;
                    } else if (belongs_to_AB_cycle2[city] == NULL_CYCLE) {
                        belongs_to_AB_cycle2[city] = i;
                    } else {
                        throw std::runtime_error("City belongs to more than 2 AB cycles");
                    }
                }
            }
            
            // 無効ABサイクルを隣接する有効ABサイクルと統合する
            for (size_t i = 0; i < city_count; ++i) {
                if (belongs_to_AB_cycle1[i] != NULL_CYCLE && belongs_to_AB_cycle2[i] == NULL_CYCLE) {
                    // AB_cyclesには、すべての有効ABサイクルが含まれているので、
                    // 一方しか記録されていないならば、もう一方は無効ABサイクルである
                    size_t AB_cycle_index = belongs_to_AB_cycle1[i];
                    size_t v1 = i;
                    
                    // 有効ABサイクルに属している方の頂点
                    size_t v_belonging_to_AB_cycle = 0;
                    // 無効ABサイクルは親間で一致している辺で構成されるので
                    // そうではない方を見ればよい
                    if (parent1[v1][0] != parent2[v1][0] && parent1[v1][0] != parent2[v1][1]) {
                        v_belonging_to_AB_cycle = parent1[v1][0];
                    } else if (parent1[v1][1] != parent2[v1][0] && parent1[v1][1] != parent2[v1][1]) {
                        v_belonging_to_AB_cycle = parent1[v1][1];
                    } else {
                        throw std::runtime_error("Invalid AB cycle");
                    }
                    
                    // 無効ABサイクルをたどって、その頂点を有効ABサイクルAB_cycle_indexに追加する
                    while (true) {
                        belongs_to_AB_cycle2[v1] = AB_cycle_index;
                        size_t next_v1 = parent1[v1][0];
                        if (next_v1 == v_belonging_to_AB_cycle) {
                            next_v1 = parent1[v1][1];
                        }
                        
                        if (belongs_to_AB_cycle1[next_v1] == NULL_CYCLE) {
                            belongs_to_AB_cycle1[next_v1] = AB_cycle_index;
                        } else if (belongs_to_AB_cycle2[next_v1] == NULL_CYCLE) {
                            belongs_to_AB_cycle2[next_v1] = AB_cycle_index;
                            break; // 無効ABサイクルの系列の終端に到達した
                        } else {
                            throw std::runtime_error("Invalid AB cycle");
                        }
                        
                        v_belonging_to_AB_cycle = v1;
                        v1 = next_v1;
                    }
                }
            }
            
            // 初期化
            auto& c_vertex_count = *c_vertex_count_ptr;
            c_vertex_count.resize(cycle_count, 0);

            auto& shared_vertex_count = *shared_vertex_count_ptr;
            shared_vertex_count.resize(cycle_count);
            for (size_t i = 0; i < cycle_count; ++i) {
                shared_vertex_count[i].resize(cycle_count, 0);
            }
            
            // C頂点の数と共有頂点の数をカウント
            for (size_t i = 0; i < city_count; ++i) {
                if (belongs_to_AB_cycle1[i] != belongs_to_AB_cycle2[i]) {
                    ++c_vertex_count[belongs_to_AB_cycle1[i]];
                    ++c_vertex_count[belongs_to_AB_cycle2[i]];
                    if (belongs_to_AB_cycle1[i] != NULL_CYCLE && belongs_to_AB_cycle2[i] != NULL_CYCLE) {
                        ++shared_vertex_count[belongs_to_AB_cycle1[i]][belongs_to_AB_cycle2[i]];
                        ++shared_vertex_count[belongs_to_AB_cycle2[i]][belongs_to_AB_cycle1[i]];
                    }
                }
            }
        }
        
        using e_set_index_vector_ptr = mpi::ObjectPool<std::vector<size_t>>::pooled_unique_ptr;
        
        e_set_index_vector_ptr search_e_set_with_tabu_search(size_t center_ab_cycle_index,
                                            mpi::ObjectPool<std::vector<size_t>>& any_size_vector_pool,
                                            std::mt19937& rng) {

            using namespace std;
            
            auto best_e_set_ptr = any_size_vector_pool.acquire_unique();
            vector<size_t>& best_e_set = *best_e_set_ptr;
            set_initial_e_set(best_e_set, center_ab_cycle_index, rng);

            vector<size_t> const& c_vertex_count = *c_vertex_count_ptr;
            vector<vector<size_t>> const& shared_vertex_count = *shared_vertex_count_ptr;

            auto shared_vertex_count_with_e_set_ptr = any_size_vector_pool.acquire_unique();
            vector<size_t>& shared_vertex_count_with_e_set = *shared_vertex_count_with_e_set_ptr;
            auto included_in_e_set_ptr = any_size_vector_pool.acquire_unique();
            vector<size_t>& included_in_e_set = *included_in_e_set_ptr;
            auto tabu_list_ptr = any_size_vector_pool.acquire_unique();
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
    private:
        void set_initial_e_set(std::vector<size_t>& e_set,
                                size_t center_ab_cycle_index,
                                std::mt19937& rng) {
            using namespace std;
            vector<vector<size_t>> const& shared_vertex_count = *shared_vertex_count_ptr;

            uniform_int_distribution<size_t> dist01(0, 1);

            e_set.clear();
            e_set.reserve(cycle_count);
            e_set.push_back(center_ab_cycle_index);
            for (size_t i = 0; i < cycle_count; ++i) {
                if (shared_vertex_count[center_ab_cycle_index][i] > 0) {
                    if (dist01(rng) == 0) {
                        e_set.push_back(i);
                    }
                }
            }
        }
        size_t city_count;
        size_t cycle_count;
        mpi::ObjectPool<std::vector<size_t>>::pooled_unique_ptr c_vertex_count_ptr;
        mpi::ObjectPool<std::vector<std::vector<size_t>>>::pooled_unique_ptr shared_vertex_count_ptr;
    };
    
    std::vector<Child> edge_assembly_crossover_block2(const Individual& parent1, const Individual& parent2, size_t children_size,
                                            eax::Environment& env, std::mt19937& rng) {
        auto& tsp = env.tsp;
        auto& adjacency_matrix = tsp.adjacency_matrix;
        auto& NN_list = tsp.NN_list;
        using namespace std;

        const size_t n = parent1.size();

        auto path_ptr = env.object_pools.vector_of_tsp_size_pool.acquire_unique();
        auto pos_ptr = env.object_pools.vector_of_tsp_size_pool.acquire_unique();
        vector<size_t>& path = *path_ptr;
        vector<size_t>& pos = *pos_ptr;
        for (size_t i = 0, prev = 0, current = 0; i < n; ++i) {
            path[i] = current;
            pos[current] = i;
            size_t next = parent1[current][0];
            if (next == prev) {
                next = parent1[current][1];
            }
            prev = current;
            current = next;
        }

        start_timer("find_AB_cycles");
        vector<ABCycle_ptr> AB_cycles = find_AB_cycles(numeric_limits<size_t>::max(), parent1, parent2, rng, env.object_pools.any_size_vector_pool, env.object_pools.vector_of_tsp_size_pool, env.object_pools.doubly_linked_list_pool, env.object_pools.LRIS_pool);
        end_timer("find_AB_cycles");
        
        sort(AB_cycles.begin(), AB_cycles.end(), [](const ABCycle_ptr& a, const ABCycle_ptr& b) {
            return a->size() > b->size();
        });
        
        Block2Strategy block2_strategy(parent1, parent2, AB_cycles, n, env.object_pools.vector_of_tsp_size_pool, env.object_pools.any_size_vector_pool, env.object_pools.any_size_2d_vector_pool);
        
        children_size = min(children_size, AB_cycles.size());

        vector<Child> children;
        auto working_individual = env.object_pools.intermediate_individual_pool.acquire_unique();
        working_individual->assign(parent1);
        for (size_t child_index = 0; child_index < children_size; ++child_index) {
            auto selected_AB_cycles_indices_ptr = block2_strategy.search_e_set_with_tabu_search(child_index, env.object_pools.any_size_vector_pool, rng);
            auto& selected_AB_cycles_indices = *selected_AB_cycles_indices_ptr;
            
            // if (selected_AB_cycles_indices.size() == AB_cycles.size()) {
            //     continue; // 全てのABサイクルを選択している場合はスキップ
            // }
            
            auto selected_AB_cycles_view = selected_AB_cycles_indices | views::transform([&AB_cycles](size_t index) -> const ABCycle& {
                return *AB_cycles[index];
            });
            start_timer("create_relaxed_individual");
            working_individual->apply_AB_cycles(selected_AB_cycles_view, pos, env);
            end_timer("create_relaxed_individual");

            start_timer("merge_sub_tours");
            merge_sub_tours(adjacency_matrix, *working_individual, path, pos, NN_list, env);
            end_timer("merge_sub_tours");

            children.emplace_back(working_individual->convert_to_child_and_revert());
        }
        
        if (children.empty()) {
            children.emplace_back(working_individual->convert_to_child_and_revert());
        }
        return children;
    }

}

std::vector<Child> edge_assembly_crossover(const Individual& parent1, const Individual& parent2, size_t children_size,
                                            eax::Environment& env, std::mt19937& rng) {
    
    switch(env.eax_type) {
        case eax::EAXType::Rand:
            return edge_assembly_crossover_Rand(parent1, parent2, children_size, env, rng);
        case eax::EAXType::N_AB:
            return edge_assembly_crossover_N_AB(parent1, parent2, children_size, env, rng);
        case eax::EAXType::Block2:
            return edge_assembly_crossover_block2(parent1, parent2, children_size, env, rng);
        default:
            throw std::runtime_error("Unknown EAX type");
    }
}

void print_time() {
    using namespace std;
    
    cout << "EAX times:" << endl;
    for (const auto& [key, value] : times) {
        cout << key << ": " << value << " seconds" << endl;
    }
    
}
}