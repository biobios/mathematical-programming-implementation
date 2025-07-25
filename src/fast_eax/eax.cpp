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

#include "limited_range_integer_set.hpp"

namespace {
struct Segment {
    size_t ID;
    size_t beginning_pos;
    size_t end_pos;
    size_t beginning_adjacent_pos;
    size_t end_adjacent_pos;
    size_t sub_tour_ID = std::numeric_limits<size_t>::max();
};

class ABCycle {
public:
    ABCycle(const std::vector<size_t>& cycle) : cycle(cycle) {}
    ABCycle(std::vector<size_t>&& cycle) : cycle(std::move(cycle)) {}
    size_t size() const {
        return cycle.size();
    }
    size_t operator[](size_t index) const {
        return cycle[index];
    }
private:
    std::vector<size_t> cycle;
};

class IntermediateIndividual {
public:
    IntermediateIndividual(const eax::Individual& parent) : working_individual(parent) {}
    eax::Child convert_to_child_and_revert() {
        revert();
        eax::Child child(std::move(modifications));
        modifications.clear();
        return child;
    }

    void swap_edges(std::pair<size_t, size_t> edge1, std::pair<size_t, size_t> edge2) {
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
    
    void change_connection(size_t v1, size_t v2, size_t new_v2) {
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
    
    void apply_AB_cycles(const std::vector<ABCycle>& AB_cycles,
                         const std::vector<size_t>& pos) {
        using namespace std;
        const size_t city_count = working_individual.size();
        // tuple <size_t, size_t, size_t> (a, b, c)
        // a: beginning or end of segment
        // b: adjacent to a
        // c: adjacent to (a - 1)
        vector<tuple<size_t, size_t, size_t>> cut_positions;
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
        for (const auto& cycle : AB_cycles) {
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

        std::sort(cut_positions.begin(), cut_positions.end());

        std::map<size_t, size_t> pos_to_segment_id;
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
        // i = cut_positions.size() - 1
        // add_segment(cut_positions.size() - 1, 0);
        
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
    }
    
    size_t sub_tour_count() const {
        return sub_tour_sizes.size();
    }
    
    std::pair<size_t, size_t> find_min_size_sub_tour() const {
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
    
    size_t find_sub_tour_containing(size_t pos) const {
        for (const auto& segment : segments) {
            if (segment.beginning_pos <= pos && pos <= segment.end_pos) {
                return segment.sub_tour_ID;
            }
        }
        throw std::runtime_error("No sub-tour found containing the position");
    }
    
    size_t get_city_pos_of_sub_tour(size_t sub_tour_id) const {
        for (const auto& segment : segments) {
            if (segment.sub_tour_ID == sub_tour_id) {
                return segment.beginning_pos;
            }
        }
        throw std::runtime_error("No sub-tour found with the given ID");
    }
    
    void merge_sub_tour(size_t sub_tour_id1, size_t sub_tour_id2) {
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
    
    const std::array<size_t, 2>& operator[](size_t index) {
        return working_individual[index];
    }
    
    const std::array<size_t, 2>& operator[](size_t index) const {
        return working_individual[index];
    }
private:
    void revert() {
        for (auto it = modifications.crbegin(); it != modifications.crend(); ++it) {
            undo(*it);
        }
    }
    
    void reset() {
        modifications.clear();
        segments.clear();
        sub_tour_sizes.clear();
    }

    void undo(const eax::Child::Modification& modification) {
        auto [v1, v2] = modification.edge1;
        size_t new_v2 = modification.new_v2;
        if (working_individual[v1][0] == new_v2) {
            working_individual[v1][0] = v2;
        } else {
            working_individual[v1][1] = v2;
        }
    }

    eax::Individual working_individual;
    std::vector<eax::Child::Modification> modifications;
    std::vector<Segment> segments;
    std::vector<size_t> sub_tour_sizes;
};

std::vector<double> times(5, 0.0);
std::vector<double> times2(5, 0.0);

ABCycle create_AB_cycle(std::vector<size_t>& finding_path,
                        size_t end_index,
                        mpi::LimitedRangeIntegerSet& cities_having_2_edges,
                        mpi::LimitedRangeIntegerSet& cities_having_just_1_edge)
{
    bool starts_with_B = finding_path.size() % 2 == 0;
    size_t start_index = finding_path.size() - 1;
    std::vector<size_t> AB_cycle;
    AB_cycle.reserve(start_index - end_index);
    
    size_t last = 0;
    if (starts_with_B) {
        last = finding_path[start_index];
        if (cities_having_2_edges.contains(last)) {
            cities_having_2_edges.erase(last);
            cities_having_just_1_edge.insert(last);
        } else if (cities_having_just_1_edge.contains(last)) {
            cities_having_just_1_edge.erase(last);
        }
        start_index -= 1;
    }
    
    for (size_t i = start_index; i > end_index; i -= 1) {
        size_t current = finding_path[i];
        AB_cycle.push_back(current);
        if (cities_having_2_edges.contains(current)) {
            cities_having_2_edges.erase(current);
            cities_having_just_1_edge.insert(current);
        } else if (cities_having_just_1_edge.contains(current)) {
            cities_having_just_1_edge.erase(current);
        }
    }
    
    if (starts_with_B) {
        AB_cycle.push_back(last);
    }
    
    finding_path.resize(end_index + 1);
    
    return ABCycle(std::move(AB_cycle));
}

std::vector<ABCycle> find_AB_cycles(size_t needs,
            const eax::Individual& parent1,
            const eax::Individual& parent2,
            std::mt19937& rng,
            size_t city_count)
{
    using namespace std;
    mpi::LimitedRangeIntegerSet cities_having_2_edges(city_count - 1, mpi::LimitedRangeIntegerSet::InitSet::Universal);
    mpi::LimitedRangeIntegerSet cities_having_just_1_edge(city_count - 1, mpi::LimitedRangeIntegerSet::InitSet::Empty);
    array<vector<uint8_t>, 2> least_recently_used_edge = {vector<uint8_t>(city_count, 0), vector<uint8_t>(city_count, 0)};
    vector<ABCycle> AB_cycles;
    
    struct {
        const eax::Individual& parent1;
        const eax::Individual& parent2;
        const eax::Individual& operator[](size_t index) const {
            return index == 0 ? parent1 : parent2;
        }
    } parents = {parent1, parent2};

    // array<const eax::Individual&, 2> parents = {parent1, parent2};
    
    uniform_int_distribution<size_t> dist_01(0, 1);
    
    vector<size_t> visited;
    vector<size_t> first_visited(city_count, 0);
    size_t current_city = numeric_limits<size_t>::max();
    
    while (cities_having_2_edges.size() > 0) {
        
        if (!(cities_having_2_edges.contains(current_city) ||
            cities_having_just_1_edge.contains(current_city))) { // 現在の都市にエッジが存在しない場合

            // 2つエッジを持つ都市からランダムに選択
            size_t rand_to_select_city = uniform_int_distribution<size_t>(0, cities_having_2_edges.size() - 1)(rng);
            current_city = *(cities_having_2_edges.begin() + rand_to_select_city);
            
            visited.clear();
            visited.push_back(current_city);
            first_visited[current_city] = 0;
        }
        
        while (true) {  // ABサイクルを一つ見つけるまでループ
            size_t prev_city = current_city;
            size_t prev_first_visited = first_visited[prev_city];
            size_t prev_edge_count = cities_having_2_edges.contains(current_city) ? 2 : 1;
            size_t next_edge_parent = visited.size() % 2;
            
            if (prev_edge_count == 1) { // エッジが一つなら
                // 単純に最近最も使用されていないエッジを選択
                current_city = parents[next_edge_parent][prev_city][least_recently_used_edge[next_edge_parent][prev_city]];
                // もうほかの枝は存在しないので LRU は更新しない
            } else {
                size_t selected_index = 0;
                if (prev_first_visited != visited.size() - 1) {
                    // 2回訪れた都市なら、残り一つのエッジを選択
                    // prev_first_visitedが直前じゃなければ、2回訪れたことが分かる
                    selected_index = least_recently_used_edge[next_edge_parent][prev_city];
                } else { // そうじゃないなら
                    selected_index = dist_01(rng);
                }
                
                current_city = parents[next_edge_parent][prev_city][selected_index];
                // LRU を更新
                least_recently_used_edge[next_edge_parent][prev_city] = 1 - selected_index;
            }
            
            visited.push_back(current_city);

            size_t current_edge_count = cities_having_2_edges.contains(current_city) ? 2 : 1;
            if (current_edge_count == 1) {
                // エッジの数が1なら、探索途中か、スタートにたどり着いて一周したか
                if (current_city == visited.front()) {
                    // ABサイクル構成処理
                    ABCycle&& cycle = create_AB_cycle(visited, 0, cities_having_2_edges, cities_having_just_1_edge);
                    if (cycle.size() > 2) {
                        AB_cycles.emplace_back(std::move(cycle));
                    }
                    break;
                } else {
                    continue; // 探索途中なら、次の都市へ
                }
            } else {
                // エッジの数が2なら、探索途中か、スタートにたどり着いて一周したか、途中で交差してABサイクルを構成するか
                // 次の都市のLRUを更新
                if (parents[next_edge_parent][current_city][0] == prev_city) {
                    least_recently_used_edge[next_edge_parent][current_city] = 1;
                } else {
                    least_recently_used_edge[next_edge_parent][current_city] = 0;
                }
                if (current_city == visited.front()) {
                    if ((visited.size() + 1) % 2 == 0) { // Bで出てAで帰ってきた
                        // ABサイクル構成処理
                        ABCycle&& cycle = create_AB_cycle(visited, 0, cities_having_2_edges, cities_having_just_1_edge);
                        if (cycle.size() > 2) {
                            AB_cycles.emplace_back(std::move(cycle));
                        }
                        break;
                    } else { // Bで出てBで帰ってきた
                        continue;
                    }
                } else if (first_visited[current_city] != 0 && (visited.size() - first_visited[current_city] + 1) % 2 == 0) {
                    // 交差している　かつ　ABサイクルを構成するなら
                    ABCycle&& cycle = create_AB_cycle(visited, first_visited[current_city], cities_having_2_edges, cities_having_just_1_edge);
                    if (cycle.size() > 2) {
                        AB_cycles.emplace_back(std::move(cycle));
                    }
                    break;
                } else if (first_visited[current_city] != 0) {
                    continue;
                } else {
                    first_visited[current_city] = visited.size() - 1;
                    continue;
                }
            }
        }
        
        if (AB_cycles.size() >= needs) {
            return AB_cycles; // 必要な数のABサイクルが見つかった
        }
    }
    
    while (cities_having_just_1_edge.size() > 0) {
        size_t start_city = *(cities_having_just_1_edge.begin());

        // 1つのエッジを持つ都市からABサイクルを構成する
        vector<size_t> visited = {start_city};

        size_t current_city = start_city;
        while (true) {
            size_t next_edge_parent = visited.size() % 2;
            size_t selected_index = least_recently_used_edge[next_edge_parent][current_city];
            size_t next_city = parents[next_edge_parent][current_city][selected_index];
            visited.push_back(next_city);
            if (next_city == start_city) 
                break;
            current_city = next_city;
        }
        
        ABCycle&& cycle = create_AB_cycle(visited, 0, cities_having_2_edges, cities_having_just_1_edge);
        if (cycle.size() > 2) {
            AB_cycles.emplace_back(std::move(cycle));
        }
        
        if (AB_cycles.size() >= needs) {
            return AB_cycles; // 必要な数のABサイクルが見つかった
        }
    }
    
    return AB_cycles;
}

void merge_sub_tours(const std::vector<std::vector<int64_t>>& adjacency_matrix,
            IntermediateIndividual& child,
            const std::vector<size_t>& path,
            const std::vector<size_t>& pos,
            const std::vector<std::vector<std::pair<int64_t, size_t>>>& NN_list)
{
    using namespace std;
    using distance_type = std::remove_cvref_t<decltype(adjacency_matrix)>::value_type::value_type;
    using edge = pair<size_t, size_t>;

    while (child.sub_tour_count() > 1) {
        auto [min_sub_tour_id, min_sub_tour_size] = child.find_min_size_sub_tour();
        size_t start_city = path[child.get_city_pos_of_sub_tour(min_sub_tour_id)];

        vector<size_t> elem_of_min_sub_tour;
        elem_of_min_sub_tour.reserve(min_sub_tour_size + 2);
        vector<bool> in_min_sub_tour(path.size(), false);
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
        
        constexpr size_t search_range = 10;
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
                        size_t connected_to_current_city = child[current_city][k];
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
        }
        
        child.swap_edges(e1, e2);
        size_t connected_to_min_sub_tour = child.find_sub_tour_containing(pos[e2.first]);
        child.merge_sub_tour(min_sub_tour_id, connected_to_min_sub_tour);
    }
}

} // namespace

namespace eax {
std::vector<Child> edge_assembly_crossover(const Individual& parent1, const Individual& parent2, size_t children_size,
                                            const eax::Environment& env, std::mt19937& rng) {

    auto& tsp = env.tsp;
    auto& adjacency_matrix = tsp.adjacency_matrix;
    auto& NN_list = tsp.NN_list;
    using namespace std;
    
    const size_t n = parent1.size();
    
    // vector<vector<size_t>> AB_cycles;
    vector<size_t> path(n);
    vector<size_t> pos(n);
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
    
    // switch (env.eax_type) {
    //     case eax::EAXType::Rand:
    //         step_2(numeric_limits<size_t>::max(), parent1, parent2, AB_cycles, rng, n);
    //         break;
    //     case eax::EAXType::N_AB:
    //         step_2(env.N_parameter * children_size, parent1, parent2, AB_cycles, rng, n);
    //         break;
    //     default:
    //         cerr << "Error: Unknown EAX type." << endl;
    //         exit(1);
    // }
    vector<ABCycle> AB_cycles;
    switch (env.eax_type) {
        case eax::EAXType::Rand:
            AB_cycles = find_AB_cycles(numeric_limits<size_t>::max(), parent1, parent2, rng, n);
            break;
        case eax::EAXType::N_AB:
            AB_cycles = find_AB_cycles(env.N_parameter * children_size, parent1, parent2, rng, n);
            break;
        default:
            cerr << "Error: Unknown EAX type." << endl;
            exit(1);
    }
    vector<size_t> AB_cycle_indices(AB_cycles.size());
    iota(AB_cycle_indices.begin(), AB_cycle_indices.end(), 0);
    shuffle(AB_cycle_indices.begin(), AB_cycle_indices.end(), rng);
    
    // step_2(parent1, parent2, AB_cycles, rng, n);
    
    vector<Child> children;
    IntermediateIndividual working_individual(parent1);
    for (size_t child_index = 0; child_index < children_size; ++child_index) {
        
        // 緩和個体を作成
        vector<Segment> segments;
        
        switch (env.eax_type) {
            case eax::EAXType::Rand:
                {
                // apply_E_set_Rand(AB_cycles, path, pos, segments, working_individual, rng);
                vector<ABCycle> selected_AB_cycles;
                vector<size_t> selected_AB_cycle_indices;
                uniform_int_distribution<size_t> dist_01(0, 1);
                for (size_t i = 0; i < AB_cycles.size(); ++i) {
                    if (dist_01(rng) == 0) {
                        selected_AB_cycles.emplace_back(std::move(AB_cycles[AB_cycle_indices[i]]));
                        selected_AB_cycle_indices.push_back(AB_cycle_indices[i]);
                    }
                }
                working_individual.apply_AB_cycles(selected_AB_cycles, pos);
                // もとに戻す
                for (size_t i = 0; i < selected_AB_cycle_indices.size(); ++i) {
                    AB_cycles[selected_AB_cycle_indices[i]] = std::move(selected_AB_cycles[i]);
                }
                }
                break;
            case eax::EAXType::N_AB:
                // apply_E_set_N_AB(AB_cycles, path, pos, segments, working_individual, env.N_parameter, rng);
                {
                vector<ABCycle> selected_AB_cycles;
                for (size_t i = 0; i < env.N_parameter && i < AB_cycles.size(); ++i) {
                    size_t index = (child_index * env.N_parameter + i) % AB_cycles.size();
                    selected_AB_cycles.emplace_back(AB_cycles[AB_cycle_indices[index]]);
                }
                working_individual.apply_AB_cycles(selected_AB_cycles, pos);
                }
                break;
            default:
                cerr << "Error: Unknown EAX type." << endl;
                exit(1);
        }

        // if (segments.empty()) {
        //     cerr << "Error: segments is empty." << endl;
        //     exit(1);
        // }

        // step_5_and_6(adjacency_matrix, segments, path, pos, working_individual, n, NN_list);
        merge_sub_tours(adjacency_matrix, working_individual, path, pos, NN_list);
        
        children.emplace_back(working_individual.convert_to_child_and_revert());

    }
    return children;
}

void print_time() {
    using namespace std;
    cout << "Step 1: " << times[0] << " seconds" << endl;
    cout << "Step 2: " << times[1] << " seconds" << endl;
    cout << "Step 3 and 4: " << times[2] << " seconds" << endl;
    cout << "Step 5: " << times[3] << " seconds" << endl;
    cout << "Step 6: " << times[4] << " seconds" << endl;
    cout << "Total: " << accumulate(times.begin(), times.end(), 0.0) << " seconds" << endl;
    
    cout << "Step 6 (total): " << times2[0] << " seconds" << endl;
    cout << "Step 6 (finding min cycle): " << times2[1] << " seconds" << endl;
    cout << "Step 6 (finding min cost edge): " << times2[2] << " seconds" << endl;
    cout << "Step 6 (merging cycles): " << times2[3] << " seconds" << endl;
}
}