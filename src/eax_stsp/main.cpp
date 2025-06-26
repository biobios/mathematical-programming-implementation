#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <sstream>
#include <cmath>
#include <random>
#include <algorithm>
#include <set>
#include <map>
#include <array>
#include <chrono>
#include <unordered_set>
#include <unordered_map>
#include <list>

#include "elitist_recombination.hpp"
#include "tsp_loader.hpp"
#include "population_initializer.hpp"

double calc_fitness(const std::vector<size_t>& path, const std::vector<std::vector<double>>& adjacency_matrix){
    double distance = 0.0;
    for (size_t i = 0; i < path.size() - 1; ++i) {
        distance += adjacency_matrix[path[i]][path[i + 1]];
    }
    // 最後の都市から最初の都市への距離を加算
    distance += adjacency_matrix[path.back()][path.front()];
    return 1 / distance;
}

void two_opt_swap(std::vector<size_t>& path, size_t i, size_t j) {
    // iとjの間の部分を逆順にする
    using namespace std;
    std::reverse(path.begin() + i, path.begin() + j + 1);
}

void apply_2opt(std::vector<size_t>& path, const std::vector<std::vector<double>>& adjacency_matrix) {
    using namespace std;
    size_t n = path.size();
    bool improved = true;
    while (improved) {
        improved = false;
        for (size_t i = 0; i < path.size() - 1 && !improved; ++i) {
            for (size_t j = i + 1; j < path.size(); ++j) {
                double old_length = adjacency_matrix[path[i]][path[i + 1]] + adjacency_matrix[path[j]][path[(j + 1) % n]];
                double new_length = adjacency_matrix[path[i]][path[j]] + adjacency_matrix[path[i + 1]][path[(j + 1) % n]];
                
                if (new_length < old_length) {
                    two_opt_swap(path, i + 1, j);
                    improved = true;
                    break;
                }
            }
        }
    }
}

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

template <>
struct std::hash<edge> {
    size_t operator()(const edge& e) const {
        return std::hash<size_t>()(e.v1) ^ (std::hash<size_t>()(e.v2) << 1);
    }
};

std::vector<std::vector<size_t>> edge_assembly_crossover(const std::vector<size_t>& parent1, const std::vector<size_t>& parent2, size_t children_size,
                                            const std::vector<std::vector<double>>& adjacency_matrix, std::mt19937& rng) {
    
    using namespace std;
    
    
    using edge_with_parent = pair<edge, bool>; // from_parent1 : bool
    
    size_t n = parent1.size();
    vector<array<size_t, 2>> adjacency_list_parent1(n);
    vector<array<size_t, 2>> adjacency_list_parent2(n);
    set<edge> parent1_edges;

    for (size_t i = 0; i < parent1.size(); ++i) {
        size_t next_i = (i + 1) % n;
        adjacency_list_parent1[parent1[i]][0] = parent1[next_i];
        adjacency_list_parent1[parent1[next_i]][1] = parent1[i];
        adjacency_list_parent2[parent2[i]][0] = parent2[next_i];
        adjacency_list_parent2[parent2[next_i]][1] = parent2[i];
        parent1_edges.insert(edge(parent1[i], parent1[next_i]));
    }
    
    vector<vector<edge_with_parent>> AB_cycles;
    
    vector<array<size_t, 2>> adjacency_list_parent1_copied = adjacency_list_parent1;
    vector<array<size_t, 2>> adjacency_list_parent2_copied = adjacency_list_parent2;
    size_t rest_edge_count = n;
    vector<size_t> prob(n, 1);
    do {

        size_t current_city = discrete_distribution<size_t>(prob.begin(), prob.end())(rng);
        vector<size_t> visited_parent1;
        vector<size_t> visited_parent2;
        visited_parent2.push_back(current_city);
        do {
            if (visited_parent1.size() < visited_parent2.size()) {
                size_t selected_index = uniform_int_distribution<size_t>(0, 1)(rng);
                if (adjacency_list_parent1_copied[current_city][selected_index] == n) {
                    selected_index = 1 - selected_index; // 反転
                }

                size_t prev_city = current_city;
                current_city = adjacency_list_parent1_copied[current_city][selected_index];
                visited_parent1.push_back(current_city);
                adjacency_list_parent1_copied[prev_city][selected_index] = n;
                if (adjacency_list_parent1_copied[current_city][0] == prev_city) {
                    adjacency_list_parent1_copied[current_city][0] = n;
                } else {
                    adjacency_list_parent1_copied[current_city][1] = n;
                }
                
                rest_edge_count -= 1;
                if (adjacency_list_parent1_copied[current_city][0] == n && adjacency_list_parent1_copied[current_city][1] == n) {
                    prob[current_city] = 0; // この都市はもう訪問しない
                }

                if (adjacency_list_parent1_copied[prev_city][0] == n && adjacency_list_parent1_copied[prev_city][1] == n) {
                    prob[prev_city] = 0; // 前の都市も訪問しない
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
                    AB_cycles.push_back(move(AB_cycle));
            }else {
                size_t selected_index = uniform_int_distribution<size_t>(0, 1)(rng);
                if (adjacency_list_parent2_copied[current_city][selected_index] == n) {
                    selected_index = 1 - selected_index; // 反転
                }

                size_t prev_city = current_city;
                current_city = adjacency_list_parent2_copied[current_city][selected_index];
                visited_parent2.push_back(current_city);
                adjacency_list_parent2_copied[prev_city][selected_index] = n;
                if (adjacency_list_parent2_copied[current_city][0] == prev_city) {
                    adjacency_list_parent2_copied[current_city][0] = n;
                } else {
                    adjacency_list_parent2_copied[current_city][1] = n;
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
                    AB_cycles.push_back(move(AB_cycle));
            }
        }while (!visited_parent1.empty());
    }while (rest_edge_count > 0);
    
    vector<vector<size_t>> children(children_size);
    for (size_t child_index = 0; child_index < children_size; ++child_index) {
        
        // // sizeが2以下のABサイクルを除外
        // for (auto it = AB_cycles.begin(); it != AB_cycles.end();) {
        //     if (it->size() <= 2) {
        //         it = AB_cycles.erase(it);
        //     } else {
        //         ++it;
        //     }
        // }
        

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
        
        // // ABサイクルをランダムに選択して、E-setを作成
        // // set<edge_with_parent> E_set;
        // vector<size_t> AB_cycle_indices(AB_cycles.size());
        // std::iota(AB_cycle_indices.begin(), AB_cycle_indices.end(), 0); // 0からN-1までの整数を初期化
        // std::shuffle(AB_cycle_indices.begin(), AB_cycle_indices.end(), rng); // ランダムにシャッフル
        // // 使用するABサイクルの数をランダムに決定(1以上N-1以下)
        // std::uniform_int_distribution<size_t> dist(1, AB_cycles.size() - 1);
        // size_t num_cycles = dist(rng);
        
        // // 緩和個体を作成
        // unordered_multiset<edge> relaxed_individual_edges(parent1_edges.begin(), parent1_edges.end());
        // for (size_t i = 0; i < num_cycles; ++i) {
        //     for (const auto& [edge, from_parent1] : AB_cycles[AB_cycle_indices[i]]) {
        //         if (from_parent1) {
        //             auto it = relaxed_individual_edges.find(edge);
        //             relaxed_individual_edges.erase(it);
        //         } else {
        //             relaxed_individual_edges.insert(edge);
        //         }
        //     }
        // }
        
        // 緩和個体を作成
        unordered_multiset<edge> relaxed_individual_edges(parent1_edges.begin(), parent1_edges.end());
        uniform_int_distribution<size_t> dist_01(0, 1);
        for (const auto& AB_cycle : AB_cycles) {
            if (dist_01(rng) == 0)
                continue; // このABサイクルは使用しない
            for (const auto& [e, from_parent1] : AB_cycle) {
                if (from_parent1) {
                    auto it = relaxed_individual_edges.find(e);
                    relaxed_individual_edges.erase(it);
                } else {
                    relaxed_individual_edges.insert(e);
                }
            }
        }
        
        // 部分巡回路の配列に変換
        vector<vector<edge>> partial_cycles;
        unordered_map<size_t, vector<size_t>> child_adjacency_list;
        for (const auto& e : relaxed_individual_edges) {
            child_adjacency_list[e.v1].push_back(e.v2);
            child_adjacency_list[e.v2].push_back(e.v1);
        }

        while (!child_adjacency_list.empty()) {
            vector<edge> cycle;
            size_t current_city = child_adjacency_list.begin()->first;

            while (child_adjacency_list.contains(current_city)) {
                auto next_city = child_adjacency_list[current_city].back();
                // リストから現在のエッジを削除
                child_adjacency_list[current_city].pop_back();
                // 逆向きも削除
                auto it = find(child_adjacency_list[next_city].begin(), child_adjacency_list[next_city].end(), current_city);
                child_adjacency_list[next_city].erase(it);

                if (child_adjacency_list[current_city].empty()) {
                    child_adjacency_list.erase(current_city);
                }
                
                if (child_adjacency_list[next_city].empty()) {
                    child_adjacency_list.erase(next_city);
                }
                cycle.push_back({current_city, next_city});
                current_city = next_city;
            }

            partial_cycles.push_back(move(cycle));
        }
        
        while (partial_cycles.size() > 1) {
            // 最小の部分巡回路を見つける
            size_t min_cycle_index = 0;
            for (size_t i = 1; i < partial_cycles.size(); ++i) {
                if (partial_cycles[i].size() < partial_cycles[min_cycle_index].size()) {
                    min_cycle_index = i;
                }
            }

            // 最小の部分巡回路と接続する部分巡回路を見つける
            edge e1;
            edge e2;
            size_t e1_index = 0;
            size_t e2_index = 0;
            // 順方向接続か逆方向接続か
            bool forward_connection = true;
            double min_cost = std::numeric_limits<double>::max();
            size_t min_cost_index = 0;

            for (size_t i = 0; i < partial_cycles.size(); ++i) {
                if (i == min_cycle_index) continue;
                
                
                // for (const auto& edge1 : partial_cycles[min_cycle_index]) {
                //     for (const auto& edge2 : partial_cycles[i]) {
                for (size_t j = 0; j < partial_cycles[min_cycle_index].size(); ++j) {
                    const auto& edge1 = partial_cycles[min_cycle_index][j];
                    for (size_t k = 0; k < partial_cycles[i].size(); ++k) {
                        const auto& edge2 = partial_cycles[i][k];
                        // double cost = - adjacency_matrix[edge1.from][edge1.to] - adjacency_matrix[edge2.from][edge2.to]
                        //             + adjacency_matrix[edge1.from][edge2.to] + adjacency_matrix[edge2.from][edge1.to];
                        
                        // 2通り試す
                        double forward_cost = - adjacency_matrix[edge1.v1][edge1.v2] - adjacency_matrix[edge2.v1][edge2.v2]
                                    + adjacency_matrix[edge1.v1][edge2.v2] + adjacency_matrix[edge2.v1][edge1.v2];
                        
                        double reverse_cost = - adjacency_matrix[edge1.v1][edge1.v2] - adjacency_matrix[edge2.v1][edge2.v2]
                                    + adjacency_matrix[edge1.v1][edge2.v1] + adjacency_matrix[edge2.v2][edge1.v2];
                        
                        if (forward_cost < min_cost || reverse_cost < min_cost) {
                            min_cost = std::min(forward_cost, reverse_cost);
                            e1 = edge1;
                            e2 = edge2;
                            e1_index = j;
                            e2_index = k;
                            min_cost_index = i;
                            forward_connection = (forward_cost < reverse_cost);
                        }
                    }
                }
            }
            // // e1とe2をそれぞれ削除して、接続する辺をpartial_cycles[min_cycle_index]に追加
            // partial_cycles[min_cycle_index].erase(partial_cycles[min_cycle_index].find(e1));
            // partial_cycles[min_cost_index].erase(partial_cycles[min_cost_index].find(e2));
            if (forward_connection) {
                // partial_cycles[min_cycle_index].insert({e1.v1, e2.v2});
                // partial_cycles[min_cycle_index].insert({e2.v1, e1.v2});
                partial_cycles[min_cycle_index][e1_index] = {e1.v1, e2.v2};
                partial_cycles[min_cost_index][e2_index] = {e2.v1, e1.v2};
            } else {
                // partial_cycles[min_cycle_index].insert({e1.v1, e2.v1});
                // partial_cycles[min_cycle_index].insert({e2.v2, e1.v2});
                partial_cycles[min_cycle_index][e1_index] = {e1.v1, e2.v1};
                partial_cycles[min_cost_index][e2_index] = {e2.v2, e1.v2};
            }
            // partial_cycles[min_cost_index]をmin_cycle_indexにマージ
            // partial_cycles[min_cycle_index].append_range(partial_cycles[min_cost_index]);
            partial_cycles[min_cycle_index].reserve(partial_cycles[min_cycle_index].size() + partial_cycles[min_cost_index].size());
            for (auto& e : partial_cycles[min_cost_index]) {
                partial_cycles[min_cycle_index].push_back(move(e));
            }

            // 末尾の部分巡回路をmin_cost_indexに移動して、削除
            if (min_cost_index < partial_cycles.size() - 1) {
                std::swap(partial_cycles[min_cost_index], partial_cycles.back());
            }
            partial_cycles.pop_back();
        }
        
        // 最後に残った部分巡回路をchild_pathに変換
        const vector<edge>& final_cycle = partial_cycles.front();
        // まず、隣接リストに変換
        vector<vector<size_t>> final_adjacency_list(final_cycle.size());
        for (const auto& e : final_cycle) {
            final_adjacency_list[e.v1].push_back(e.v2);
            final_adjacency_list[e.v2].push_back(e.v1);
        }
        // そして、child_path(パス表現)に変換
        size_t current_city = 0;
        size_t prev_sity = 0;
        vector<size_t>& child_path = children[child_index];
        child_path.resize(final_adjacency_list.size());
        for (size_t i = 0; i < child_path.size(); ++i) {
            child_path[i] = current_city;
            size_t next_city = final_adjacency_list[current_city][0];
            if (next_city == prev_sity) {
                next_city = final_adjacency_list[current_city][1];
            }
            prev_sity = current_city;
            current_city = next_city;
        }
        
    }
    return children;
}

int main()
{
    using namespace std;
    // TSPファイルの読み込み
    string file_name = "rat575.tsp";
    tsp::TSP tsp = tsp::TSP_Loader::load_tsp(file_name);
    cout << "TSP Name: " << tsp.name << endl;
    cout << "Distance Type: " << tsp.distance_type << endl;
    cout << "Number of Cities: " << tsp.city_count << endl;
    
    // 試行回数
    constexpr size_t trials = 30;
    // 世代数
    constexpr size_t generations = 300;
    // 乱数生成器(グローバル)
    // mt19937 rng;
    mt19937 rng(545404204);
    // 集団サイズ
    size_t population_size = 0;
    if (tsp.name == "rat575") {
        population_size = 300;
    } else if (tsp.name == "att532") {
        population_size = 250;
    } else {
        cerr << "Unsupported TSP file: " << tsp.name << endl;
        return 1;
    }
    // 初期集団生成器
    tsp::PopulationInitializer population_initializer(population_size, tsp.city_count, 
    [&tsp](vector<size_t>& individual) {
        // 2-opt法を適用
        apply_2opt(individual, tsp.adjacency_matrix);
    });
    
    using Individual = vector<size_t>;
    
    // 1試行にかかった時間
    vector<double> trial_times(trials, 0.0);
    // 各試行のベストの経路長
    vector<double> best_path_lengths(trials, 0.0);
    // 各試行のベストに到達した最初の世代
    vector<size_t> generation_of_best(trials, 0);
    
    for (size_t trial = 0; trial < trials; ++trial) {
        cout << "Trial " << trial + 1 << " of " << trials << endl;
        // 乱数生成器(ローカル)
        // グローバルで初期化
        mt19937::result_type seed = rng();
        vector<Individual> population = population_initializer.initialize_population(seed, "initial_population_cache_" + to_string(seed) + "_for_" + file_name);
        cout << "Initial population created." << endl;

        // 終了判定関数
        // 世代数に達するか、収束するまで実行
        struct {
            size_t generation = 0;
            size_t max_generations = generations;
            
            double best_fitness = 0.0;
            size_t generation_of_reached_best = 0;
            
            bool operator()([[maybe_unused]]const vector<Individual>& population, const vector<double>& fitness_values, [[maybe_unused]]const vector<vector<double>>& adjacency_matrix) {
                
                double ave_fitness = 0.0;
                double max_fitness = 0.0;
                double min_fitness = std::numeric_limits<double>::max();
                for (const auto& fitness : fitness_values) {
                    ave_fitness += fitness;
                    max_fitness = std::max(max_fitness, fitness);
                    min_fitness = std::min(min_fitness, fitness);
                }
                ave_fitness /= fitness_values.size();
                
                if (max_fitness > best_fitness) {
                    best_fitness = max_fitness;
                    generation_of_reached_best = generation;
                }
                
                // 世代数を増やす
                ++generation;
                // 終了条件を満たすかどうかを判定
                return generation > max_generations || max_fitness == min_fitness;
            }
        } end_condition;
        
        // 隣接行列
        auto& adjacency_matrix = tsp.adjacency_matrix;
        
        // // 乱数生成器再初期化
        // local_rng.seed(seed);
        mt19937 local_rng(seed);
        
        // 計測開始
        auto start_time = chrono::high_resolution_clock::now();

        // 世代交代モデル ElitistRecombinationを使用して、遺伝的アルゴリズムを実行
        vector<Individual> result = mpi::genetic_algorithm::ElitistRecombination<100>(population, end_condition, calc_fitness, edge_assembly_crossover, adjacency_matrix, local_rng);
        
        auto end_time = chrono::high_resolution_clock::now();
        trial_times[trial] = chrono::duration<double>(end_time - start_time).count();
        
        best_path_lengths[trial] = 1.0 / end_condition.best_fitness;
        generation_of_best[trial] = end_condition.generation_of_reached_best;
    }
    
    // 全試行中のベストとその解に到達した試行の数を出力する
    double best_path_length = *min_element(best_path_lengths.begin(), best_path_lengths.end());
    size_t best_path_reached_count = count_if(best_path_lengths.begin(), best_path_lengths.end(),
                                        [best_path_length](double length) { return length == best_path_length; });
    cout << "Best path length: " << best_path_length << endl;
    cout << "Number of trials that reached the best path: " << best_path_reached_count << endl;

    // ベストの平均を出力
    double average_best_path_length = accumulate(best_path_lengths.begin(), best_path_lengths.end(), 0.0) / trials;
    cout << "Average best path length: " << average_best_path_length << endl;

    // ベストに到達した最初の世代の平均を出力
    double average_generation_of_best = accumulate(generation_of_best.begin(), generation_of_best.end(), 0.0) / trials;
    cout << "Average generation of best path: " << average_generation_of_best << endl;

    // 各試行の経過時間を出力
    cout << "Trial times (seconds): ";
    for (const auto& time : trial_times) {
        cout << time << " ";
    }
    cout << endl;
    return 0;
}