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

namespace {
struct edge {
    size_t v1;
    size_t v2;
    edge(size_t v1, size_t v2) {
        if (v1 < v2) {
            this->v1 = v1;
            this->v2 = v2;
        } else {
            this->v1 = v2;
            this->v2 = v1;
        }
    }
    edge() : v1(0), v2(0) {}
    // setに格納するための比較演算子
    bool operator<(const edge& other) const {
        return std::tie(v1, v2) < std::tie(other.v1, other.v2);
    }
    
    bool operator==(const edge& other) const {
        return v1 == other.v1 && v2 == other.v2;
    }
};

struct Segment {
    size_t ID;
    size_t beginning_pos;
    size_t end_pos;
    size_t beginning_adjacent_pos;
    size_t end_adjacent_pos;
    size_t sub_tour_ID = std::numeric_limits<size_t>::max();
};

std::vector<double> times(5, 0.0);
std::vector<double> times2(5, 0.0);

void step_2(const eax::Individual& parent1,
            const eax::Individual& parent2,
            std::vector<std::vector<size_t>>& AB_cycles,
            std::mt19937& rng,
            size_t n)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    using namespace std;
    size_t rest_edge_count = n;
    vector<array<bool, 2>> edge_used_parent1(n, {false, false});
    vector<array<bool, 2>> edge_used_parent2(n, {false, false});
    vector<size_t> rest_cities(n);
    vector<size_t> city_indices(n);
    iota(rest_cities.begin(), rest_cities.end(), 0);
    iota(city_indices.begin(), city_indices.end(), 0);
    do {

        // size_t current_city = discrete_distribution<size_t>(prob.begin(), prob.end())(rng);
        size_t current_city = rest_cities[uniform_int_distribution<size_t>(0, rest_cities.size() - 1)(rng)];
        vector<size_t> visited_parent1;
        vector<size_t> visited_parent2;
        visited_parent2.push_back(current_city);
        do {
            if (visited_parent1.size() < visited_parent2.size()) {
                size_t selected_index = uniform_int_distribution<size_t>(0, 1)(rng);
                // if (adjacency_list_parent1[current_city][selected_index] == n) {
                if (edge_used_parent1[current_city][selected_index]) {
                    selected_index = 1 - selected_index; // 反転
                }

                size_t prev_city = current_city;
                current_city = parent1[current_city][selected_index];
                visited_parent1.push_back(current_city);
                // adjacency_list_parent1[prev_city][selected_index] = n;
                edge_used_parent1[prev_city][selected_index] = true;
                if (parent1[current_city][0] == prev_city) {
                    // adjacency_list_parent1[current_city][0] = n;
                    edge_used_parent1[current_city][0] = true;
                } else {
                    // adjacency_list_parent1[current_city][1] = n;
                    edge_used_parent1[current_city][1] = true;
                }
                
                rest_edge_count -= 1;
                if (edge_used_parent1[current_city][0] && edge_used_parent1[current_city][1]) {
                    // prob[current_city] = 0; // この都市はもう訪問しない
                    size_t back = rest_cities.back();
                    rest_cities[city_indices[current_city]] = back;
                    city_indices[back] = city_indices[current_city];
                    rest_cities.pop_back();
                }

                if (edge_used_parent1[prev_city][0] && edge_used_parent1[prev_city][1]) {
                    // prob[prev_city] = 0; // 前の都市も訪問しない
                    size_t back = rest_cities.back();
                    rest_cities[city_indices[prev_city]] = back;
                    city_indices[back] = city_indices[prev_city];
                    rest_cities.pop_back();
                }

                size_t found_loop_index = visited_parent1.size();
                for (size_t i = 0; i < visited_parent1.size() - 1; ++i) {
                    if (visited_parent1[i] == current_city) {
                        found_loop_index = i;
                        break;
                    }
                }

                if (found_loop_index == visited_parent1.size()) {
                    continue;
                }

                vector<size_t> AB_cycle;
                AB_cycle.reserve((visited_parent1.size() - found_loop_index) * 2);
                for (size_t i = found_loop_index + 1; i < visited_parent1.size(); ++i) {
                    AB_cycle.push_back(visited_parent2[i]);
                    AB_cycle.push_back(visited_parent1[i]);
                }
                visited_parent1.resize(found_loop_index + 1);
                visited_parent2.resize(found_loop_index + 1);
                if (AB_cycle.size() > 2)
                    AB_cycles.emplace_back(move(AB_cycle));
            }else {
                size_t selected_index = uniform_int_distribution<size_t>(0, 1)(rng);
                if (edge_used_parent2[current_city][selected_index]) {
                    selected_index = 1 - selected_index; // 反転
                }

                size_t prev_city = current_city;
                current_city = parent2[current_city][selected_index];
                visited_parent2.push_back(current_city);
                // adjacency_list_parent2[prev_city][selected_index] = n;
                edge_used_parent2[prev_city][selected_index] = true;
                // if (adjacency_list_parent2[current_city][0] == prev_city) {
                if (parent2[current_city][0] == prev_city) {
                    // adjacency_list_parent2[current_city][0] = n;
                    edge_used_parent2[current_city][0] = true;
                } else {
                    // adjacency_list_parent2[current_city][1] = n;
                    edge_used_parent2[current_city][1] = true;
                }

                size_t found_loop_index = visited_parent2.size();
                for (size_t i = 0; i < visited_parent2.size() - 1; ++i) {
                    if (visited_parent2[i] == current_city) {
                        found_loop_index = i;
                        break;
                    }
                }
                if (found_loop_index == visited_parent2.size()) {
                    continue;
                }
                vector<size_t> AB_cycle;
                AB_cycle.reserve((visited_parent2.size() - found_loop_index) * 2);
                for (size_t i = found_loop_index + 1; i < visited_parent2.size(); ++i) {
                    AB_cycle.push_back(visited_parent2[i - 1]);
                    AB_cycle.push_back(visited_parent1[i - 1]);
                }
                visited_parent1.resize(found_loop_index);
                visited_parent2.resize(found_loop_index + 1);
                if (AB_cycle.size() > 2)
                    AB_cycles.emplace_back(move(AB_cycle));
            }
        }while (!visited_parent1.empty());
    }while (rest_edge_count > 0);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    times[1] += std::chrono::duration<double>(end_time - start_time).count();
    
}

void apply_E_set_Rand(const std::vector<std::vector<size_t>>& AB_cycles,
                  const std::vector<size_t>& path,
                  const std::vector<size_t>& pos,
                  std::vector<Segment>& segments,
                  eax::Individual& child,
                  std::mt19937& rng)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    using namespace std;
    uniform_int_distribution<size_t> dist_01(0, 1);
    vector<pair<size_t, size_t>> cut_positions; // (pos, adjacent_pos)
    bool cut_between_first_and_last = false;
    for (const auto& AB_cycle : AB_cycles) {
        if (dist_01(rng) == 0)
            continue; // このABサイクルは使用しない
    
        for (size_t i = 1; i < AB_cycle.size(); i += 2) {
            size_t next_index = (i + 1) % AB_cycle.size();
            size_t v1 = AB_cycle[i - 1];
            size_t v2 = AB_cycle[i];
            size_t v3 = AB_cycle[next_index];

            if (child[v2][0] == v1) {
                child[v2][0] = v3;
            } else {
                child[v2][1] = v3;
            }

            if ((v1 == path.front() && v2 == path.back()) || 
                (v2 == path.front() && v1 == path.back())) {
                cut_between_first_and_last = true;
            }
        }

        for (size_t i = 0; i < AB_cycle.size(); i += 2) {
            size_t prev_index = (i + AB_cycle.size() - 1) % AB_cycle.size();
            size_t next_index = (i + 1) % AB_cycle.size();
            size_t v1 = AB_cycle[next_index];
            size_t v2 = AB_cycle[i];
            size_t v3 = AB_cycle[prev_index];

            if (child[v2][0] == v1) {
                child[v2][0] = v3;
            } else {
                child[v2][1] = v3;
            }
        }

        for (size_t i = 1; i < AB_cycle.size(); i += 2) {
            size_t next_index = (i + 1) % AB_cycle.size();
            size_t v1 = AB_cycle[i];
            size_t v2 = AB_cycle[next_index];
            cut_positions.emplace_back(pos[v1], pos[v2]);
            cut_positions.emplace_back(pos[v2], pos[v1]);
        }
        
    }
    
    
    std::sort(cut_positions.begin(), cut_positions.end());
    segments.reserve(cut_positions.size() / 2);
    size_t segment_id = 0;
    size_t start = 0;
    if (!cut_between_first_and_last) {
        if (cut_positions.empty()) {
            return;
        }
        start = 1;
        cut_positions.emplace_back(cut_positions.front().first, cut_positions.front().second);
    }
    for (size_t i = start; i < cut_positions.size() - 1; i += 2, segment_id++) {
        Segment segment;
        segment.ID = segment_id;
        segment.beginning_pos = cut_positions[i].first;
        segment.end_pos = cut_positions[i + 1].first;
        segment.beginning_adjacent_pos = cut_positions[i].second;
        segment.end_adjacent_pos = cut_positions[i + 1].second;
        segments.push_back(segment);
    }
    
    
    auto end_time = std::chrono::high_resolution_clock::now();
    times[2] += std::chrono::duration<double>(end_time - start_time).count();
    
}

void apply_E_set_N_AB(const std::vector<std::vector<size_t>>& AB_cycles,
                  const std::vector<size_t>& path,
                  const std::vector<size_t>& pos,
                  std::vector<Segment>& segments,
                  eax::Individual& child,
                  const size_t n_parameter,
                  std::mt19937& rng)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    using namespace std;
    // uniform_int_distribution<size_t> dist_01(0, 1);
    // uniform_int_distribution<size_t> dist(0, AB_cycles.size() - 1);
    vector<size_t> indicies(AB_cycles.size());
    iota(indicies.begin(), indicies.end(), 0);
    shuffle(indicies.begin(), indicies.end(), rng);
    vector<pair<size_t, size_t>> cut_positions; // (pos, adjacent_pos)
    bool cut_between_first_and_last = false;
    // auto& AB_cycle = AB_cycles[dist(rng)];
    // for (const auto& AB_cycle : AB_cycles) {
    for (size_t i = 0; i < n_parameter && i < AB_cycles.size(); ++i) {
        const auto& AB_cycle = AB_cycles[indicies[i]];
    // if (dist_01(rng) == 0)
    //     continue; // このABサイクルは使用しない
    
        for (size_t i = 1; i < AB_cycle.size(); i += 2) {
            size_t next_index = (i + 1) % AB_cycle.size();
            size_t v1 = AB_cycle[i - 1];
            size_t v2 = AB_cycle[i];
            size_t v3 = AB_cycle[next_index];

            if (child[v2][0] == v1) {
                child[v2][0] = v3;
            } else {
                child[v2][1] = v3;
            }

            if ((v1 == path.front() && v2 == path.back()) || 
                (v2 == path.front() && v1 == path.back())) {
                cut_between_first_and_last = true;
            }
        }

        for (size_t i = 0; i < AB_cycle.size(); i += 2) {
            size_t prev_index = (i + AB_cycle.size() - 1) % AB_cycle.size();
            size_t next_index = (i + 1) % AB_cycle.size();
            size_t v1 = AB_cycle[next_index];
            size_t v2 = AB_cycle[i];
            size_t v3 = AB_cycle[prev_index];

            if (child[v2][0] == v1) {
                child[v2][0] = v3;
            } else {
                child[v2][1] = v3;
            }
        }

        for (size_t i = 1; i < AB_cycle.size(); i += 2) {
            size_t next_index = (i + 1) % AB_cycle.size();
            size_t v1 = AB_cycle[i];
            size_t v2 = AB_cycle[next_index];
            cut_positions.emplace_back(pos[v1], pos[v2]);
            cut_positions.emplace_back(pos[v2], pos[v1]);
        }
        
    }
    
    
    std::sort(cut_positions.begin(), cut_positions.end());
    segments.reserve(cut_positions.size() / 2);
    size_t segment_id = 0;
    size_t start = 0;
    if (!cut_between_first_and_last) {
        if (cut_positions.empty()) {
            return;
        }
        start = 1;
        cut_positions.emplace_back(cut_positions.front().first, cut_positions.front().second);
    }
    for (size_t i = start; i < cut_positions.size() - 1; i += 2, segment_id++) {
        Segment segment;
        segment.ID = segment_id;
        segment.beginning_pos = cut_positions[i].first;
        segment.end_pos = cut_positions[i + 1].first;
        segment.beginning_adjacent_pos = cut_positions[i].second;
        segment.end_adjacent_pos = cut_positions[i + 1].second;
        segments.push_back(segment);
    }
    
    
    auto end_time = std::chrono::high_resolution_clock::now();
    times[2] += std::chrono::duration<double>(end_time - start_time).count();
}

void step_5_and_6(const std::vector<std::vector<int64_t>>& adjacency_matrix,
            std::vector<Segment>& segments,
            const std::vector<size_t>& path,
            const std::vector<size_t>& pos,
            eax::Individual& child,
            size_t n,
            const std::vector<std::vector<std::pair<int64_t, size_t>>>& NN_list)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    using namespace std;
    map<size_t, size_t> belongs_to_segment;
    for (auto& segment : segments) {
        belongs_to_segment[segment.beginning_pos] = segment.ID;
        belongs_to_segment[segment.end_pos] = segment.ID;
    }
    
    vector<size_t> unvisited_segments;
    vector<size_t> unvisited_segments_indices;
    unvisited_segments.reserve(segments.size());
    unvisited_segments_indices.reserve(segments.size());
    for (size_t i = 0; i < segments.size(); ++i) {
        unvisited_segments.push_back(i);
        unvisited_segments_indices.push_back(i);
    }
    
    size_t sub_tour_ID = 0;
    vector<size_t> sub_tour_sizes;
    vector<set<size_t>> sub_tours_segments;
    
    while (unvisited_segments.size() > 0) {
        set<size_t> sub_tour_segments_set;
        size_t current_segment_index = unvisited_segments.back();
        size_t prev_pos = segments[current_segment_index].beginning_pos;
        size_t sub_tour_size = 0;
        while (segments[current_segment_index].sub_tour_ID == numeric_limits<size_t>::max()) {
            sub_tour_segments_set.insert(current_segment_index);
            auto& segment = segments[current_segment_index];
            segment.sub_tour_ID = sub_tour_ID;

            unvisited_segments[unvisited_segments_indices[current_segment_index]] = unvisited_segments.back();
            unvisited_segments_indices[unvisited_segments.back()] = unvisited_segments_indices[current_segment_index];
            unvisited_segments.pop_back();
            
            if (segment.beginning_pos <= segment.end_pos) {
                sub_tour_size += segment.end_pos - segment.beginning_pos + 1;
            } else {
                sub_tour_size += n - segment.beginning_pos + segment.end_pos + 1;
            }
            
            size_t current_pos = segment.end_pos;
            size_t next_pos = segment.end_adjacent_pos;
            if (next_pos == prev_pos) {
                current_pos = segment.beginning_pos;
                next_pos = segment.beginning_adjacent_pos;               
            }
            
            current_segment_index = belongs_to_segment[next_pos];
            prev_pos = current_pos;
        }
        
        sub_tour_sizes.push_back(sub_tour_size);
        sub_tours_segments.emplace_back(move(sub_tour_segments_set));
        sub_tour_ID++;
    }
    
    
    using distance_type = std::remove_cvref_t<decltype(adjacency_matrix)>::value_type::value_type;
    // vector<size_t> belongs_to_cycle(n, n); // 各都市がどの部分巡回路に属するかを記録
    // vector<vector<size_t>> cycles_cities; // 各部分巡回路に属する都市のリスト
    // vector<pair<size_t, size_t>> double_linked_list(n, {n, n});
    // size_t remaining_edges = n;
    // size_t cycle_index = 0;
    
    // while (remaining_edges > 0) {
    //     vector<size_t> cycle_cities;
    //     size_t current_city = 0;
        
    //     for (size_t i = 0; i < n; ++i) {
    //         if (!child_adjacency_list[i].empty()) {
    //             current_city = i;
    //             break;
    //         }
    //     }
    //     size_t prev_city = 0;
    //     while (!child_adjacency_list[current_city].empty()) {
    //         auto next_city = child_adjacency_list[current_city].back();
    //         // リストから現在のエッジを削除
    //         child_adjacency_list[current_city].pop_back();
    //         // 逆向きも削除
    //         auto it = find(child_adjacency_list[next_city].begin(), child_adjacency_list[next_city].end(), current_city);
    //         child_adjacency_list[next_city].erase(it);

    //         belongs_to_cycle[current_city] = cycle_index;
    //         cycle_cities.push_back(current_city);
    //         double_linked_list[current_city] = {prev_city, next_city};
    //         prev_city = current_city;
    //         current_city = next_city;
    //         remaining_edges -= 1;
    //     }
        
    //     double_linked_list[current_city].first = prev_city;

    //     cycles_cities.emplace_back(move(cycle_cities));
    //     cycle_index += 1;
    // }

    auto end_time = std::chrono::high_resolution_clock::now();
    times[3] += std::chrono::duration<double>(end_time - start_time).count();
    start_time = std::chrono::high_resolution_clock::now();
    
    
    auto start_time2 = std::chrono::high_resolution_clock::now();
    
    while (sub_tours_segments.size() > 1) {
        auto start_time3 = std::chrono::high_resolution_clock::now();
        // 最小の部分巡回路を見つける
        size_t min_sub_tour_index = 0;
        size_t min_sub_tour_size = std::numeric_limits<size_t>::max();
        for (size_t i = 0; i < sub_tour_sizes.size(); ++i) {
            if (sub_tour_sizes[i] < min_sub_tour_size) {
                min_sub_tour_size = sub_tour_sizes[i];
                min_sub_tour_index = i;
            }
        }
        vector<bool> in_min_sub_tour(n, false);
        for (auto segment_index : sub_tours_segments[min_sub_tour_index]) {
            auto& segment = segments[segment_index];
            if (segment.beginning_pos <= segment.end_pos) {
                for (size_t pos = segment.beginning_pos; pos <= segment.end_pos; ++pos) {
                    in_min_sub_tour[path[pos]] = true;
                }
            } else {
                for (size_t pos = segment.beginning_pos; pos < n; ++pos) {
                    in_min_sub_tour[path[pos]] = true;
                }
                for (size_t pos = 0; pos <= segment.end_pos; ++pos) {
                    in_min_sub_tour[path[pos]] = true;
                }
            }
        }
        auto end_time3 = std::chrono::high_resolution_clock::now();
        times2[1] += std::chrono::duration<double>(end_time3 - start_time3).count();
        auto start_time4 = std::chrono::high_resolution_clock::now();
        
        auto& min_sub_tour_set = sub_tours_segments[min_sub_tour_index];
        
        constexpr size_t search_range = 10;
        size_t start = 0;
        edge e1 = {0, 0};
        edge e2 = {0, 0};
        distance_type min_cost = std::numeric_limits<distance_type>::max();
        bool forward_connection = true;
        while (e1.v1 == 0 && e1.v2 == 0) {
            // if (start >= n) {
            //     cerr << "start >= n, breaking" << endl;
            //     cerr << "min_sub_tour_set.size() = " << min_sub_tour_set.size() << endl;
            //     cerr << "segments.size() = " << segments.size() << endl;
            //     cerr << "sub_tour_sizes.size() = " << sub_tour_sizes.size() << endl;
            //     cerr << "sub_tours_segments.size() = " << sub_tours_segments.size() << endl;
            //     cerr << "sub_tour_ID = " << sub_tour_ID << endl;
            //     cerr << "n = " << n << endl;
            //     // cerr << "min_sub_tour_set = {" << endl;
            //     // for (const auto& segment_index : min_sub_tour_set) {
            //     //     cerr << "  Segment ID: " << segments[segment_index].ID << ", "
            //     //          << "beginning_pos: " << segments[segment_index].beginning_pos << ", "
            //     //          << "end_pos: " << segments[segment_index].end_pos << endl;
            //     // }
            //     // cerr << "}" << endl;
            //     cerr << "sub_tours_segments = {" << endl;
            //     for (const auto& segment_set : sub_tours_segments) {
            //         cerr << "  {";
            //         for (const auto& segment_index : segment_set) {
            //             cerr << segment_index << " : ";
            //             cerr << "Segment ID: " << segments[segment_index].ID << ", "
            //                  << "beginning_pos: " << segments[segment_index].beginning_pos << ", "
            //                  << "end_pos: " << segments[segment_index].end_pos << endl;
            //         }
            //         cerr << "  }," << endl;
            //     }
            //     cerr << "}" << endl;
            //     exit(1);
            // }
            for (auto segment_index : min_sub_tour_set) {
                auto& segment = segments[segment_index];
                vector<pair<size_t, size_t>> ranges;
                if (segment.beginning_pos <= segment.end_pos) {
                    ranges.emplace_back(segment.beginning_pos, segment.end_pos);
                } else {
                    ranges.emplace_back(segment.beginning_pos, n - 1);
                    ranges.emplace_back(0, segment.end_pos);
                }

                for (const auto& [start_pos, end_pos] : ranges) {
                    // for (size_t v1_index : cycles_cities[cycle_index]) {
                    for (size_t v1_pos = start_pos; v1_pos <= end_pos; ++v1_pos) {
                        size_t v1_index = path[v1_pos];

                        size_t limit = std::min(start + search_range, NN_list[v1_index].size());
                        for (size_t i = start; i < limit; ++i) {
                            size_t v2_index = NN_list[v1_index][i].second;
                            // size_t v2_pos = pos[v2_index];
                            // bool is_v2_in_the_segment = false;
                            // if (min_cycle_set.contains(belongs_to_cycle[v2_index])) {
                            //     continue;
                            // }
                            // for (auto segment_index2 : min_sub_tour_set) {
                            //     auto& segment2 = segments[segment_index2];
                            //     if (segment2.beginning_pos <= segment2.end_pos) {
                            //         if (v2_pos >= segment2.beginning_pos && v2_pos <= segment2.end_pos) {
                            //             is_v2_in_the_segment = true;
                            //             break;
                            //         }
                            //     } else {
                            //         if (v2_pos >= segment2.beginning_pos || v2_pos <= segment2.end_pos) {
                            //             is_v2_in_the_segment = true;
                            //             break;
                            //         }
                            //     }
                            // }
                            
                            // if (is_v2_in_the_segment) {
                            //     continue;
                            // }
                            if (in_min_sub_tour[v2_index]) {
                                continue; // v2は最小の部分巡回路に含まれている
                            }

                            // 8通りの接続のコストを計算
                            array<size_t, 2> connected_city_pair_v1 = child[v1_index];
                            array<size_t, 2> connected_city_pair_v2 = child[v2_index];
                            
                            array<edge, 2> edges1 = {
                                edge(v1_index, connected_city_pair_v1[0]),
                                edge(v1_index, connected_city_pair_v1[1])
                            };
                            
                            array<edge, 2> edges2 = {
                                edge(v2_index, connected_city_pair_v2[0]),
                                edge(v2_index, connected_city_pair_v2[1])
                            };
                            
                            for (const auto& edge1 : edges1) {
                                for (const auto& edge2 : edges2) {
                                    distance_type forward_cost = - adjacency_matrix[edge1.v1][edge1.v2] - adjacency_matrix[edge2.v1][edge2.v2]
                                                + adjacency_matrix[edge1.v1][edge2.v2] + adjacency_matrix[edge2.v1][edge1.v2];

                                    distance_type reverse_cost = - adjacency_matrix[edge1.v1][edge1.v2] - adjacency_matrix[edge2.v1][edge2.v2]
                                                + adjacency_matrix[edge1.v1][edge2.v1] + adjacency_matrix[edge2.v2][edge1.v2];
                                    
                                    if (forward_cost < min_cost || reverse_cost < min_cost) {
                                        e1 = edge1;
                                        e2 = edge2;
                                        min_cost = std::min(forward_cost, reverse_cost);
                                        forward_connection = (forward_cost < reverse_cost);
                                        // if (edge1.v1 == edge2.v1 || edge1.v1 == edge2.v2 || edge1.v2 == edge2.v1 || edge1.v2 == edge2.v2) {
                                        //     cerr << "Error: e1 and e2 are the same edge." << endl;
                                        //     cerr << "e1 = (" << edge1.v1 << ", " << edge1.v2 << "), e2 = (" << edge2.v1 << ", " << edge2.v2 << ")" << endl;
                                        //     cerr << "e1 pos = (" << pos[edge1.v1] << ", " << pos[edge1.v2] << "), e2 pos = (" << pos[edge2.v1] << ", " << pos[edge2.v2] << ")" << endl;
                                        //     cerr << "sub_tours_segments = {" << endl;
                                        //     for (const auto& segment_set : sub_tours_segments) {
                                        //         cerr << "  {";
                                        //         for (const auto& segment_index : segment_set) {
                                        //             cerr << segment_index << " : ";
                                        //             cerr << "Segment ID: " << segments[segment_index].ID << ", "
                                        //                  << "beginning_pos: " << segments[segment_index].beginning_pos << ", "
                                        //                  << "end_pos: " << segments[segment_index].end_pos << ", "
                                        //                  << "beginning_adjacent_pos: " << segments[segment_index].beginning_adjacent_pos << ", "
                                        //                  << "end_adjacent_pos: " << segments[segment_index].end_adjacent_pos << endl;
                                        //         }
                                        //         cerr << "  }," << endl;
                                        //     }
                                        //     cerr << "}" << endl;
                                        //     cerr << "child = {" << endl;
                                        //     for (size_t i = 0; i < child.size(); ++i) {
                                        //         cerr << "  " << pos[i] << ": {" << pos[child[i][0]] << ", " << pos[child[i][1]] << "}" << endl;
                                        //     }
                                        //     cerr << "}" << endl;
                                        //     exit(1);
                                        // }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            start += search_range;
            // 見つからなかったら次の範囲を探す
        }
        
        auto end_time4 = std::chrono::high_resolution_clock::now();
        times2[2] += std::chrono::duration<double>(end_time4 - start_time4).count();
        auto start_time5 = std::chrono::high_resolution_clock::now();
            
        // 見つかった辺を使って部分巡回路を接続
        if (forward_connection) {
    //         e1.v2 -> e2.v2;
            if (child[e1.v1][0] == e1.v2) {
                child[e1.v1][0] = e2.v2;
            } else {
                child[e1.v1][1] = e2.v2;
            }

    //         e1.v1 -> e2.v1;
            if (child[e1.v2][0] == e1.v1) {
                child[e1.v2][0] = e2.v1;
            } else {
                child[e1.v2][1] = e2.v1;
            }
            
    //         e2.v2 -> e1.v2;
            if (child[e2.v1][0] == e2.v2) {
                child[e2.v1][0] = e1.v2;
            } else {
                child[e2.v1][1] = e1.v2;
            }
            
    //         e2.v1 -> e1.v1;
            if (child[e2.v2][0] == e2.v1) {
                child[e2.v2][0] = e1.v1;
            } else {
                child[e2.v2][1] = e1.v1;
            }
        } else {
    //         e1.v2 -> e2.v1;
            if (child[e1.v1][0] == e1.v2) {
                child[e1.v1][0] = e2.v1;
            } else {
                child[e1.v1][1] = e2.v1;
            }

    //         e1.v1 -> e2.v2;
            if (child[e1.v2][0] == e1.v1) {
                child[e1.v2][0] = e2.v2;
            } else {
                child[e1.v2][1] = e2.v2;
            }
            
    //         e2.v2 -> e1.v1;
            if (child[e2.v1][0] == e2.v2) {
                child[e2.v1][0] = e1.v1;
            } else {
                child[e2.v1][1] = e1.v1;
            }
            
    //         e2.v1 -> e1.v2;
            if (child[e2.v2][0] == e2.v1) {
                child[e2.v2][0] = e1.v2;
            } else {
                child[e2.v2][1] = e1.v2;
            }
        }
        
        // if (e1.v1 == e2.v1 || e1.v1 == e2.v2 || e1.v2 == e2.v1 || e1.v2 == e2.v2) {
        //     cerr << "Error: e1 and e2 are the same edge." << endl;
        //     cerr << "e1 = (" << e1.v1 << ", " << e1.v2 << "), e2 = (" << e2.v1 << ", " << e2.v2 << ")" << endl;
        //     cerr << "sub_tours_segments = {" << endl;
        //     for (const auto& segment_set : sub_tours_segments) {
        //         cerr << "  {";
        //         for (const auto& segment_index : segment_set) {
        //             cerr << segment_index << " : ";
        //             cerr << "Segment ID: " << segments[segment_index].ID << ", "
        //                  << "beginning_pos: " << segments[segment_index].beginning_pos << ", "
        //                  << "end_pos: " << segments[segment_index].end_pos << ", "
        //                  << "beginning_adjacent_pos: " << segments[segment_index].beginning_adjacent_pos << ", "
        //                  << "end_adjacent_pos: " << segments[segment_index].end_adjacent_pos << endl;
        //         }
        //         cerr << "  }," << endl;
        //     }
        //     cerr << "}" << endl;
        //     cerr << "v1_pos = " << pos[e1.v1] << ", v2_pos = " << pos[e1.v2] << endl;
        //     cerr << "v1_index = " << path[pos[e1.v1]] << ", v2_index = " << path[pos[e1.v2]] << endl;
        //     cerr << "child = {" << endl;
        //     for (size_t i = 0; i < child.size(); ++i) {
        //         cerr << "  " << i << ": {" << child[i][0] << ", " << child[i][1] << "}" << endl;
        //     }
        //     cerr << "}" << endl;
        //     exit(1);
        // }
        
        // 接続した部分巡回路を削除
        size_t connect_index_with_min_sub_tour = numeric_limits<size_t>::max();
        size_t connect_city_pos = pos[e2.v2];
        for (size_t i = 0; i < sub_tours_segments.size(); ++i) {
            for (auto segment_index : sub_tours_segments[i]) {
                auto& segment = segments[segment_index];
                if (segment.beginning_pos <= segment.end_pos) {
                    if (segment.beginning_pos <= connect_city_pos && segment.end_pos >= connect_city_pos) {
                        connect_index_with_min_sub_tour = i;
                        break;
                    }
                } else {
                    if (segment.beginning_pos <= connect_city_pos || segment.end_pos >= connect_city_pos) {
                        connect_index_with_min_sub_tour = i;
                        break;
                    }
                }
            }
            
            if (connect_index_with_min_sub_tour != numeric_limits<size_t>::max()) {
                break;
            }
        }
        
        // if (connect_index_with_min_sub_tour == numeric_limits<size_t>::max()) {
        //     cerr << "Error: connect_index_with_min_sub_tour is not found." << endl;
        //     cerr << "e2.v2 = " << e2.v2 << endl;
        //     cerr << "connect_city_pos = " << connect_city_pos << endl;
        //     cerr << "sub_tours_segments.size() = " << sub_tours_segments.size() << endl;
        //     cerr << "sub_tour_sizes.size() = " << sub_tour_sizes.size() << endl;
        //     cerr << "segments = {" << endl;
        //     for (const auto& segment : segments) {
        //         cerr << "  Segment ID: " << segment.ID << ", "
        //              << "beginning_pos: " << segment.beginning_pos << ", "
        //              << "end_pos: " << segment.end_pos << ", "
        //              << "beginning_adjacent_pos: " << segment.beginning_adjacent_pos << ", "
        //              << "end_adjacent_pos: " << segment.end_adjacent_pos << ", "
        //              << "sub_tour_ID: " << segment.sub_tour_ID << endl;
        //     }
        //     cerr << "}" << endl;
            
        //     cerr << "sub_tours_segments = {" << endl;
        //     for (size_t i = 0; i < sub_tours_segments.size(); ++i) {
        //         cerr << "  Segment ID: " << i << ", segments: {";
        //         for (const auto& segment_index : sub_tours_segments[i]) {
        //             cerr << segment_index << ", ";
        //         }
        //         cerr << "}" << endl;
        //     }
        //     cerr << "}" << endl;
        //     exit(1);
        // }
        
        // partial_cycle_indices[connect_index_with_min_cycle].merge(partial_cycle_indices[min_cycle_index]);
        // if (min_cycle_index < partial_cycle_indices.size() - 1) {
        //     std::swap(partial_cycle_indices[min_cycle_index], partial_cycle_indices.back());
        // }
        
        // partial_cycle_indices.pop_back();
        
        sub_tour_sizes[connect_index_with_min_sub_tour] += sub_tour_sizes[min_sub_tour_index];
        sub_tours_segments[connect_index_with_min_sub_tour].merge(sub_tours_segments[min_sub_tour_index]);
        if (min_sub_tour_index < sub_tours_segments.size() - 1) {
            std::swap(sub_tours_segments[min_sub_tour_index], sub_tours_segments.back());
            std::swap(sub_tour_sizes[min_sub_tour_index], sub_tour_sizes.back());
        }
        
        // vector<uint64_t> appeared_count(segments.size(), 0);
        // for (size_t sub_tour_index = 0; sub_tour_index < sub_tours_segments.size(); ++sub_tour_index) {
        //     for (auto segment_index : sub_tours_segments[sub_tour_index]) {
        //         appeared_count[segment_index]++;
        //     }
        // }

        // for (size_t i = 0; i < segments.size(); ++i) {
        //     if (appeared_count[i] != 1) {
        //         cerr << "Error: segment " << i << " is not appeared in any sub tour." << endl;
        //         cerr << "segments[" << i << "] = {"
        //              << "ID: " << segments[i].ID << ", "
        //              << "beginning_pos: " << segments[i].beginning_pos << ", "
        //              << "end_pos: " << segments[i].end_pos << ", "
        //              << "beginning_adjacent_pos: " << segments[i].beginning_adjacent_pos << ", "
        //              << "end_adjacent_pos: " << segments[i].end_adjacent_pos
        //              << ", sub_tour_ID: " << segments[i].sub_tour_ID
        //              << "}" << endl;
        //         cerr << "v2_index = " << path[connect_city_pos] << endl;
        //         exit(1);
        //     }
        // }
        
        sub_tours_segments.pop_back();
        sub_tour_sizes.pop_back();
        
        auto end_time5 = std::chrono::high_resolution_clock::now();
        times2[3] += std::chrono::duration<double>(end_time5 - start_time5).count();
        
    }
    
    auto end_time2 = std::chrono::high_resolution_clock::now();
    times2[0] += std::chrono::duration<double>(end_time2 - start_time2).count();
    
    // child_path.resize(n);
    
    // size_t prev_city = 0;
    // size_t current_city = 0;
    // for (size_t i = 0; i < n; ++i) {
    //     child_path[i] = current_city;
    //     auto next_city = double_linked_list[current_city].first;
    //     if (next_city == prev_city) {
    //         next_city = double_linked_list[current_city].second;
    //     }
    //     prev_city = current_city;
    //     current_city = next_city;
    // }
    
    end_time = std::chrono::high_resolution_clock::now();
    times[4] += std::chrono::duration<double>(end_time - start_time).count();
}
} // namespace

template <>
struct std::hash<edge> {
    size_t operator()(const edge& e) const {
        return std::hash<size_t>()(e.v1) ^ (std::hash<size_t>()(e.v2) << 1);
    }
};

namespace eax {
std::vector<Individual> edge_assembly_crossover(const Individual& parent1, const Individual& parent2, size_t children_size,
                                            const eax::Environment& env, std::mt19937& rng) {

    auto& tsp = env.tsp;
    auto& adjacency_matrix = tsp.adjacency_matrix;
    auto& NN_list = tsp.NN_list;
    using namespace std;
    
    // using edge_with_parent = pair<edge, bool>; // from_parent1 : bool
    
    const size_t n = parent1.size();
    
    vector<vector<size_t>> AB_cycles;
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
    
    
    step_2(parent1, parent2, AB_cycles, rng, n);
    
    
    vector<Individual> children(children_size, parent1);
    for (size_t child_index = 0; child_index < children_size; ++child_index) {
        
        
        // ABサイクルが1個以下なら、parent1かparent2をランダムに返す
        if (AB_cycles.size() <= 1) {
            uniform_int_distribution<size_t> dist(0, 1);
            if (dist(rng) == 0) {
                children[child_index] = parent1;
            } else {
                children[child_index] = parent2;
            }
            continue;
        }
        
        // 緩和個体を作成
        Individual& child = children[child_index];
        vector<Segment> segments;
        
        switch (env.eax_type) {
            case eax::EAXType::Rand:
                apply_E_set_Rand(AB_cycles, path, pos, segments, child, rng);
                break;
            case eax::EAXType::N_AB:
                apply_E_set_N_AB(AB_cycles, path, pos, segments, child, env.N_parameter, rng);
                break;
            default:
                cerr << "Error: Unknown EAX type." << endl;
                exit(1);
        }

        if (segments.empty()) {
            uniform_int_distribution<size_t> dist(0, 1);
            if (dist(rng) == 0) {
                children[child_index] = parent1;
            } else {
                children[child_index] = parent2;
            }
        }

        step_5_and_6(adjacency_matrix, segments, path, pos, child, n, NN_list);
        
        vector<uint64_t> visit_count(n, 0);
        size_t prev_city = 0;
        size_t current_city = 0;
        for (size_t i = 0; i < n; ++i) {
            visit_count[current_city]++;
            size_t next_city = child[current_city][0];
            if (next_city == prev_city) {
                next_city = child[current_city][1];
            }
            prev_city = current_city;
            current_city = next_city;
        }
        
        // 各都市の訪問回数が1回でない場合、エラー
        for (size_t i = 0; i < n; ++i) {
            if (visit_count[i] != 1) {
                cerr << "Error: city " << i << " visited " << visit_count[i] << " times." << endl;
                cerr << "Child path: ";
                for (size_t j = 0; j < n; ++j) {
                    cerr << child[j][0] << " ";
                    if (child[j][1] != child[j][0]) {
                        cerr << child[j][1] << " ";
                    }
                }
                cerr << endl;
                exit(1);
            }
        }
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