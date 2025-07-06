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

std::vector<double> times(5, 0.0);
std::vector<double> times2(5, 0.0);

void step_1(const std::vector<size_t>& parent1, const std::vector<size_t>& parent2,
             std::vector<std::array<size_t, 2>>& adjacency_list_parent1,
             std::vector<std::array<size_t, 2>>& adjacency_list_parent2,
             std::vector<std::vector<size_t>>& prototype_child,
             size_t n) {
    auto start_time = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < n; ++i) {
        size_t next_i = (i + 1) % n;
        adjacency_list_parent1[parent1[i]][0] = parent1[next_i];
        adjacency_list_parent1[parent1[next_i]][1] = parent1[i];
        adjacency_list_parent2[parent2[i]][0] = parent2[next_i];
        adjacency_list_parent2[parent2[next_i]][1] = parent2[i];
        prototype_child[parent1[i]][0] = parent1[next_i];
        prototype_child[parent1[next_i]][1] = parent1[i];
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    times[0] += std::chrono::duration<double>(end_time - start_time).count();
}

void step_2(std::vector<std::array<size_t, 2>> adjacency_list_parent1,
            std::vector<std::array<size_t, 2>> adjacency_list_parent2,
            std::vector<std::vector<std::pair<edge, bool>>>& AB_cycles,
            std::mt19937& rng,
            size_t n)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    using namespace std;
    using edge_with_parent = pair<edge, bool>; // from_parent1 : bool
    size_t rest_edge_count = n;
    // vector<size_t> prob(n, 1);
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
                if (adjacency_list_parent1[current_city][selected_index] == n) {
                    selected_index = 1 - selected_index; // 反転
                }

                size_t prev_city = current_city;
                current_city = adjacency_list_parent1[current_city][selected_index];
                visited_parent1.push_back(current_city);
                adjacency_list_parent1[prev_city][selected_index] = n;
                if (adjacency_list_parent1[current_city][0] == prev_city) {
                    adjacency_list_parent1[current_city][0] = n;
                } else {
                    adjacency_list_parent1[current_city][1] = n;
                }
                
                rest_edge_count -= 1;
                if (adjacency_list_parent1[current_city][0] == n && adjacency_list_parent1[current_city][1] == n) {
                    // prob[current_city] = 0; // この都市はもう訪問しない
                    size_t back = rest_cities.back();
                    rest_cities[city_indices[current_city]] = back;
                    city_indices[back] = city_indices[current_city];
                    rest_cities.pop_back();
                }

                if (adjacency_list_parent1[prev_city][0] == n && adjacency_list_parent1[prev_city][1] == n) {
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

                vector<edge_with_parent> AB_cycle;
                AB_cycle.reserve((visited_parent1.size() - found_loop_index) * 2);
                for (size_t i = found_loop_index + 1; i < visited_parent1.size(); ++i) {
                    AB_cycle.push_back({{visited_parent1[i - 1], visited_parent2[i]}, false});
                    AB_cycle.push_back({{visited_parent2[i], visited_parent1[i]}, true});
                }
                visited_parent1.resize(found_loop_index + 1);
                visited_parent2.resize(found_loop_index + 1);
                if (AB_cycle.size() > 2)
                    AB_cycles.emplace_back(move(AB_cycle));
            }else {
                size_t selected_index = uniform_int_distribution<size_t>(0, 1)(rng);
                if (adjacency_list_parent2[current_city][selected_index] == n) {
                    selected_index = 1 - selected_index; // 反転
                }

                size_t prev_city = current_city;
                current_city = adjacency_list_parent2[current_city][selected_index];
                visited_parent2.push_back(current_city);
                adjacency_list_parent2[prev_city][selected_index] = n;
                if (adjacency_list_parent2[current_city][0] == prev_city) {
                    adjacency_list_parent2[current_city][0] = n;
                } else {
                    adjacency_list_parent2[current_city][1] = n;
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
                vector<edge_with_parent> AB_cycle;
                AB_cycle.reserve((visited_parent2.size() - found_loop_index) * 2);
                for (size_t i = found_loop_index + 1; i < visited_parent2.size(); ++i) {
                    AB_cycle.push_back({{visited_parent2[i - 1], visited_parent1[i - 1]}, true});
                    AB_cycle.push_back({{visited_parent1[i - 1], visited_parent2[i]}, false});
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

void step_3_and_4(std::vector<std::vector<size_t>>& child_adjacency_list,
                  const std::vector<std::vector<std::pair<edge, bool>>>& AB_cycles,
                  std::mt19937& rng)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    using namespace std;
    uniform_int_distribution<size_t> dist_01(0, 1);
    for (const auto& AB_cycle : AB_cycles) {
        if (dist_01(rng) == 0)
            continue; // このABサイクルは使用しない
        
        if (AB_cycle.front().second) {
            for (size_t i = 0; i < AB_cycle.size(); i += 2) {
                const auto& [e, from_parent1] = AB_cycle[i];
                const auto& [v1, v2] = e;
                auto it = find(child_adjacency_list[v1].begin(), child_adjacency_list[v1].end(), v2);
                child_adjacency_list[v1].erase(it);
                it = find(child_adjacency_list[v2].begin(), child_adjacency_list[v2].end(), v1);
                child_adjacency_list[v2].erase(it);
            }
            
            for (size_t i = 1; i < AB_cycle.size(); i += 2) {
                const auto& [e, from_parent1] = AB_cycle[i];
                const auto& [v1, v2] = e;
                child_adjacency_list[v1].push_back(v2);
                child_adjacency_list[v2].push_back(v1);
            }
        } else {
            for (size_t i = 1; i < AB_cycle.size(); i += 2) {
                const auto& [e, from_parent1] = AB_cycle[i];
                const auto& [v1, v2] = e;
                auto it = find(child_adjacency_list[v1].begin(), child_adjacency_list[v1].end(), v2);
                child_adjacency_list[v1].erase(it);
                it = find(child_adjacency_list[v2].begin(), child_adjacency_list[v2].end(), v1);
                child_adjacency_list[v2].erase(it);
            }

            for (size_t i = 0; i < AB_cycle.size(); i += 2) {
                const auto& [e, from_parent1] = AB_cycle[i];
                const auto& [v1, v2] = e;
                child_adjacency_list[v1].push_back(v2);
                child_adjacency_list[v2].push_back(v1);
            }
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    times[2] += std::chrono::duration<double>(end_time - start_time).count();
}

void step_5_and_6(std::vector<std::vector<size_t>>& child_adjacency_list,
            const std::vector<std::vector<int64_t>>& adjacency_matrix,
            std::vector<size_t>& child_path,
            size_t n,
            const std::vector<std::vector<std::pair<int64_t, size_t>>>& NN_list)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    using namespace std;
    using distance_type = std::remove_cvref_t<decltype(adjacency_matrix)>::value_type::value_type;
    vector<size_t> belongs_to_cycle(n, n); // 各都市がどの部分巡回路に属するかを記録
    vector<vector<size_t>> cycles_cities; // 各部分巡回路に属する都市のリスト
    vector<pair<size_t, size_t>> double_linked_list(n, {n, n});
    size_t remaining_edges = n;
    size_t cycle_index = 0;
    
    while (remaining_edges > 0) {
        vector<size_t> cycle_cities;
        size_t current_city = 0;
        
        for (size_t i = 0; i < n; ++i) {
            if (!child_adjacency_list[i].empty()) {
                current_city = i;
                break;
            }
        }
        size_t prev_city = 0;
        while (!child_adjacency_list[current_city].empty()) {
            auto next_city = child_adjacency_list[current_city].back();
            // リストから現在のエッジを削除
            child_adjacency_list[current_city].pop_back();
            // 逆向きも削除
            auto it = find(child_adjacency_list[next_city].begin(), child_adjacency_list[next_city].end(), current_city);
            child_adjacency_list[next_city].erase(it);

            belongs_to_cycle[current_city] = cycle_index;
            cycle_cities.push_back(current_city);
            double_linked_list[current_city] = {prev_city, next_city};
            prev_city = current_city;
            current_city = next_city;
            remaining_edges -= 1;
        }
        
        double_linked_list[current_city].first = prev_city;

        cycles_cities.emplace_back(move(cycle_cities));
        cycle_index += 1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    times[3] += std::chrono::duration<double>(end_time - start_time).count();
    start_time = std::chrono::high_resolution_clock::now();
    
    
    auto start_time2 = std::chrono::high_resolution_clock::now();
    
    vector<set<size_t>> partial_cycle_indices(cycles_cities.size());
    for (size_t i = 0; i < cycles_cities.size(); ++i) {
        partial_cycle_indices[i].insert(i);
    }
    
    while (partial_cycle_indices.size() > 1) {
        // 最小の部分巡回路を見つける
        size_t min_cycle_index = 0;
        size_t min_cycle_size = std::numeric_limits<size_t>::max();
        for (size_t i = 0; i < partial_cycle_indices.size(); ++i) {
            size_t size = 0;
            for (const auto& index : partial_cycle_indices[i]) {
                size += cycles_cities[index].size();
            }
            
            if (size < min_cycle_size) {
                min_cycle_size = size;
                min_cycle_index = i;
            }
        }
        
        auto& min_cycle_set = partial_cycle_indices[min_cycle_index];
        
        constexpr size_t search_range = 10;
        size_t start = 0;
        edge e1 = {0, 0};
        edge e2 = {0, 0};
        distance_type min_cost = std::numeric_limits<distance_type>::max();
        bool forward_connection = true;
        while (e1.v1 == 0 && e1.v2 == 0) {
            for (auto cycle_index : min_cycle_set) {
                for (size_t v1_index : cycles_cities[cycle_index]) {
                    size_t limit = std::min(start + search_range, NN_list[v1_index].size());
                    for (size_t i = start; i < limit; ++i) {
                        size_t v2_index = NN_list[v1_index][i].second;
                        if (min_cycle_set.contains(belongs_to_cycle[v2_index])) {
                            continue;
                        }

                        // 8通りの接続のコストを計算
                        pair<size_t, size_t> connected_city_pair_v1 = double_linked_list[v1_index];
                        pair<size_t, size_t> connected_city_pair_v2 = double_linked_list[v2_index];
                        
                        array<edge, 2> edges1 = {
                            edge(v1_index, connected_city_pair_v1.first),
                            edge(v1_index, connected_city_pair_v1.second)
                        };
                        
                        array<edge, 2> edges2 = {
                            edge(v2_index, connected_city_pair_v2.first),
                            edge(v2_index, connected_city_pair_v2.second)
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
                                }
                            }
                        }
                    }
                }
            }
            
            start += search_range;
            // 見つからなかったら次の範囲を探す
        }
        
        // 見つかった辺を使って部分巡回路を接続
        if (forward_connection) {
    //         e1.v2 -> e2.v2;
            if (double_linked_list[e1.v1].first == e1.v2) {
                double_linked_list[e1.v1].first = e2.v2;
            } else {
                double_linked_list[e1.v1].second = e2.v2;
            }

    //         e1.v1 -> e2.v1;
            if (double_linked_list[e1.v2].first == e1.v1) {
                double_linked_list[e1.v2].first = e2.v1;
            } else {
                double_linked_list[e1.v2].second = e2.v1;
            }
            
    //         e2.v2 -> e1.v2;
            if (double_linked_list[e2.v1].first == e2.v2) {
                double_linked_list[e2.v1].first = e1.v2;
            } else {
                double_linked_list[e2.v1].second = e1.v2;
            }
            
    //         e2.v1 -> e1.v1;
            if (double_linked_list[e2.v2].first == e2.v1) {
                double_linked_list[e2.v2].first = e1.v1;
            } else {
                double_linked_list[e2.v2].second = e1.v1;
            }
        } else {
    //         e1.v2 -> e2.v1;
            if (double_linked_list[e1.v1].first == e1.v2) {
                double_linked_list[e1.v1].first = e2.v1;
            } else {
                double_linked_list[e1.v1].second = e2.v1;
            }

    //         e1.v1 -> e2.v2;
            if (double_linked_list[e1.v2].first == e1.v1) {
                double_linked_list[e1.v2].first = e2.v2;
            } else {
                double_linked_list[e1.v2].second = e2.v2;
            }
            
    //         e2.v2 -> e1.v1;
            if (double_linked_list[e2.v1].first == e2.v2) {
                double_linked_list[e2.v1].first = e1.v1;
            } else {
                double_linked_list[e2.v1].second = e1.v1;
            }
            
    //         e2.v1 -> e1.v2;
            if (double_linked_list[e2.v2].first == e2.v1) {
                double_linked_list[e2.v2].first = e1.v2;
            } else {
                double_linked_list[e2.v2].second = e1.v2;
            }
        }
        
        // 接続した部分巡回路を削除
        size_t connect_index_with_min_cycle = 0;
        size_t connect_cycle_index = belongs_to_cycle[e2.v1];
        for (size_t i = 0; i < partial_cycle_indices.size(); ++i) {
            if (partial_cycle_indices[i].contains(connect_cycle_index)) {
                connect_index_with_min_cycle = i;
                break;
            }
        }
        
        partial_cycle_indices[connect_index_with_min_cycle].merge(partial_cycle_indices[min_cycle_index]);
        if (min_cycle_index < partial_cycle_indices.size() - 1) {
            std::swap(partial_cycle_indices[min_cycle_index], partial_cycle_indices.back());
        }
        
        partial_cycle_indices.pop_back();
    }
    
    
    auto end_time2 = std::chrono::high_resolution_clock::now();
    times2[0] += std::chrono::duration<double>(end_time2 - start_time2).count();
    
    child_path.resize(n);
    
    size_t prev_city = 0;
    size_t current_city = 0;
    for (size_t i = 0; i < n; ++i) {
        child_path[i] = current_city;
        auto next_city = double_linked_list[current_city].first;
        if (next_city == prev_city) {
            next_city = double_linked_list[current_city].second;
        }
        prev_city = current_city;
        current_city = next_city;
    }
    
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
std::vector<std::vector<size_t>> edge_assembly_crossover(const std::vector<size_t>& parent1, const std::vector<size_t>& parent2, size_t children_size,
                                            const tsp::TSP& tsp, std::mt19937& rng) {

    auto& adjacency_matrix = tsp.adjacency_matrix;
    auto& NN_list = tsp.NN_list;
    using namespace std;
    
    using edge_with_parent = pair<edge, bool>; // from_parent1 : bool
    
    size_t n = parent1.size();
    vector<size_t> dummy(n);
    vector<array<size_t, 2>> adjacency_list_parent1(n);
    vector<array<size_t, 2>> adjacency_list_parent2(n);
    vector<vector<size_t>> prototype_child(n, vector<size_t>(2));
    step_1(parent1, parent2, adjacency_list_parent1, adjacency_list_parent2, prototype_child, n);
    
    vector<vector<edge_with_parent>> AB_cycles;
    
    
    step_2(adjacency_list_parent1, adjacency_list_parent2, AB_cycles, rng, n);
    
    vector<vector<size_t>> children(children_size);
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
        vector<vector<size_t>> child_adjacency_list = prototype_child;
        
        step_3_and_4(child_adjacency_list, AB_cycles, rng);

        vector<size_t>& child_path = children[child_index];
        step_5_and_6(child_adjacency_list, adjacency_matrix, child_path, n, NN_list);
        
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
    
    cout << "Step 2 (detailed): " << times2[0] << " seconds" << endl;
    cout << "Step 2 (finding min cycle): " << times2[1] << " seconds" << endl;
    cout << "Step 2 (finding min cost edge): " << times2[2] << " seconds" << endl;
    cout << "Step 2 (merging cycles): " << times2[3] << " seconds" << endl;
    cout << "Total (detailed): " << accumulate(times2.begin(), times2.end(), 0.0) << " seconds" << endl;
}
}