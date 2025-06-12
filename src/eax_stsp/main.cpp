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

#include "elitist_recombination.hpp"

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
    double current_fitness = calc_fitness(path, adjacency_matrix);
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

std::vector<std::vector<size_t>> edge_assembly_crossover(const std::vector<size_t>& parent1, const std::vector<size_t>& parent2, size_t children_size,
                                            const std::vector<std::vector<double>>& adjacency_matrix, std::mt19937& rng) {
    
    using namespace std;
    
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
    };
    
    using edge_with_parent = pair<edge, bool>; // from_parent1 : bool
    
    auto has_any_edge = [](const vector<array<size_t, 2>>& adjacency_list) {
        size_t n = adjacency_list.size();
        for (const auto& edge : adjacency_list) {
            if (edge[0] != n || edge[1] != n) {
                return true; // 少なくとも1つのエッジが存在する
            }
        }
        
        return false; // すべてのエッジがnである場合、エッジは存在しない
    };

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
    
    vector<vector<size_t>> children(children_size);
    for (size_t child_index = 0; child_index < children_size; ++child_index) {
        vector<set<edge_with_parent>> AB_cycles;
        
        vector<array<size_t, 2>> adjacency_list_parent1_copied = adjacency_list_parent1;
        vector<array<size_t, 2>> adjacency_list_parent2_copied = adjacency_list_parent2;
        do {
            vector<size_t> prob;
            for (size_t i = 0; i < n; ++i) {
                if (adjacency_list_parent1_copied[i][0] != n || adjacency_list_parent1_copied[i][1] != n) {
                    prob.push_back(1);
                }else{
                    prob.push_back(0);
                }
            }
            

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

                    set<edge_with_parent> AB_cycle;
                    for (size_t i = found_loop_index + 1; i < visited_parent1.size(); ++i) {
                        AB_cycle.insert({{visited_parent1[i - 1], visited_parent2[i]}, false});
                        AB_cycle.insert({{visited_parent2[i], visited_parent1[i]}, true});
                    }
                    visited_parent1.resize(found_loop_index + 1);
                    visited_parent2.resize(found_loop_index + 1);
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
                    set<edge_with_parent> AB_cycle;
                    for (size_t i = found_loop_index + 1; i < visited_parent2.size(); ++i) {
                        AB_cycle.insert({{visited_parent2[i - 1], visited_parent1[i - 1]}, true});
                        AB_cycle.insert({{visited_parent1[i - 1], visited_parent2[i]}, false});
                    }
                    visited_parent1.resize(found_loop_index);
                    visited_parent2.resize(found_loop_index + 1);
                    AB_cycles.push_back(move(AB_cycle));
                }
            }while (!visited_parent1.empty());
            
        }while (has_any_edge(adjacency_list_parent1_copied));
        

        // sizeが2以下のABサイクルを除外
        for (auto it = AB_cycles.begin(); it != AB_cycles.end();) {
            if (it->size() <= 2) {
                it = AB_cycles.erase(it);
            } else {
                ++it;
            }
        }
        

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
        
        
        
        // ABサイクルをランダムに選択して、E-setを作成
        set<edge_with_parent> E_set;
        vector<size_t> AB_cycle_indices(AB_cycles.size());
        std::iota(AB_cycle_indices.begin(), AB_cycle_indices.end(), 0); // 0からN-1までの整数を初期化
        std::shuffle(AB_cycle_indices.begin(), AB_cycle_indices.end(), rng); // ランダムにシャッフル
        // 使用するABサイクルの数をランダムに決定(1以上N-1以下)
        std::uniform_int_distribution<size_t> dist(1, AB_cycles.size() - 1);
        size_t num_cycles = dist(rng);
        size_t merge_count = 0;
        for (size_t i = 0; i < num_cycles; ++i) {
            merge_count += AB_cycles[AB_cycle_indices[i]].size();
            E_set.merge(AB_cycles[AB_cycle_indices[i]]);
        }
        
        // 緩和個体を作成
        multiset<edge> relaxed_individual_edges(parent1_edges.begin(), parent1_edges.end());
        for (const auto& [edge, from_parent1] : E_set) {
            if (from_parent1) {
                auto it = relaxed_individual_edges.find(edge);
                relaxed_individual_edges.erase(it);
            } else {
                relaxed_individual_edges.insert(edge);
            }
        }
        
        // 部分巡回路の配列に変換
        vector<multiset<edge>> partial_cycles;
        map<size_t, vector<size_t>> child_adjacency_list;
        for (const auto& e : relaxed_individual_edges) {
            child_adjacency_list[e.v1].push_back(e.v2);
            child_adjacency_list[e.v2].push_back(e.v1);
        }
        
        
        while (!child_adjacency_list.empty()) {
            multiset<edge> cycle;
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
                cycle.insert({current_city, next_city});
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
            // 順方向接続か逆方向接続か
            bool forward_connection = true;
            double min_cost = std::numeric_limits<double>::max();
            size_t min_cost_index = 0;

            multiset<edge>& min_cycle = partial_cycles[min_cycle_index];
            for (size_t i = 0; i < partial_cycles.size(); ++i) {
                if (i == min_cycle_index) continue;
                
                for (const auto& edge1 : partial_cycles[min_cycle_index]) {
                    for (const auto& edge2 : partial_cycles[i]) {
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
                            min_cost_index = i;
                            forward_connection = (forward_cost < reverse_cost);
                        }
                    }
                }
            }
            // e1とe2をそれぞれ削除して、接続する辺をpartial_cycles[min_cycle_index]に追加
            partial_cycles[min_cycle_index].erase(partial_cycles[min_cycle_index].find(e1));
            partial_cycles[min_cost_index].erase(partial_cycles[min_cost_index].find(e2));
            if (forward_connection) {
                partial_cycles[min_cycle_index].insert({e1.v1, e2.v2});
                partial_cycles[min_cycle_index].insert({e2.v1, e1.v2});
            } else {
                partial_cycles[min_cycle_index].insert({e1.v1, e2.v1});
                partial_cycles[min_cycle_index].insert({e2.v2, e1.v2});
            }
            // partial_cycles[min_cost_index]をmin_cycle_indexにマージ
            partial_cycles[min_cycle_index].merge(partial_cycles[min_cost_index]);

            // 末尾の部分巡回路をmin_cost_indexに移動して、削除
            if (min_cost_index < partial_cycles.size() - 1) {
                std::swap(partial_cycles[min_cost_index], partial_cycles.back());
            }
            partial_cycles.pop_back();
        }
        

        // 最後に残った部分巡回路をchild_pathに変換
        const multiset<edge>& final_cycle = partial_cycles.front();
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

constexpr double calc_distance(const std::tuple<double, double>& city1, const std::tuple<double, double>& city2) {
    double xd = std::get<0>(city1) - std::get<0>(city2);
    double yd = std::get<1>(city1) - std::get<1>(city2);
    double rij = std::sqrt((xd * xd + yd * yd) / 10.0);
    double tij = int(rij + 0.5);
    if (tij < rij) return tij + 1.0;
    else return tij;
}

int main()
{
    using namespace std;
    string file_name = "att532.tsp";
    fstream file(file_name);
    if (!file.is_open()) {
        cerr << "Error: Could not open the file '" << file_name << "'." << endl;
        return 1;
    }
    
    vector<tuple<int/*city id*/, double/*x*/, double/*y*/>> city_positions;
    string line;
    // ヘッダ行を読み飛ばす(NODE_COORD_SECTIONが出てくるまで)
    while (getline(file, line)) {
        if (line.find("NODE_COORD_SECTION") != string::npos) {
            break; // NODE_COORD_SECTIONが見つかったらループを抜ける
        }
    }
    
    // "EOF"が出てくるまで都市の座標を読み込む
    while (getline(file, line)) {
        if (line.find("EOF") != string::npos) {
            break; // EOFが見つかったらループを抜ける
        }
        
        istringstream iss(line);
        int id;
        double x, y;
        if (!(iss >> id >> x >> y)) {
            cerr << "Error: Invalid line format: " << line << endl;
            continue; // 無効な行はスキップ
        }
        
        city_positions.emplace_back(id, x, y);
    }
    file.close();
    
    // 隣接行列を作成
    vector<vector<double>> adjacency_matrix(city_positions.size(), vector<double>(city_positions.size(), 0.0));
    for (size_t i = 0; i < city_positions.size(); ++i) {
        for (size_t j = 0; j < city_positions.size(); ++j) {
            if (i == j)
                continue;
            
            auto [id_i, x_i, y_i] = city_positions[i];
            auto [id_j, x_j, y_j] = city_positions[j];
            double distance = calc_distance(make_tuple(x_i, y_i), make_tuple(x_j, y_j));
            
            adjacency_matrix[i][j] = distance;
            adjacency_matrix[j][i] = distance;
        }
    }
    
    cout << "Number of cities: " << city_positions.size() << endl;
    
    // 集団サイズ
    constexpr size_t population_size = 250;
    // 世代数
    constexpr size_t generations = 300;
    // 乱数生成器
    mt19937 rng;
    
    using Individual = vector<size_t>;
    
    // 集団を初期化
    vector<Individual> population(population_size);
    
    string cache_file_name = "initial_population_cache.txt";
    // キャッシュファイルが存在する場合は、そこから初期集団を読み込む
    ifstream cache_file(cache_file_name);
    if (cache_file.is_open()) {
        cout << "Loading initial population from cache file: " << cache_file_name << endl;
        for (size_t i = 0; i < population_size; ++i) {
            population[i].resize(city_positions.size());
            for (size_t j = 0; j < city_positions.size(); ++j) {
                cache_file >> population[i][j];
            }
            // 2opt法を適用
            apply_2opt(population[i], adjacency_matrix);
            cout << "Individual " << i << " loaded from cache." << endl;
        }
        cache_file.close();
    }else {
        for (size_t i = 0; i < population_size; ++i) {
            population[i].resize(city_positions.size());
            iota(population[i].begin(), population[i].end(), 0); // 0からN-1までの整数を初期化
            // ランダムにシャッフル
            shuffle(population[i].begin(), population[i].end(), rng);
            // 2opt法を適用
            apply_2opt(population[i], adjacency_matrix);
            cout << "Individual " << i << " initialized." << endl;
        }
        
        // 初期集団をキャッシュファイルに保存
        ofstream cache_file(cache_file_name);
        if (!cache_file.is_open()) {
            cerr << "Error: Could not open cache file '" << cache_file_name << "' for writing." << endl;
            return 1;
        }
        cout << "Saving initial population to cache file: " << cache_file_name << endl;
        for (const auto& individual : population) {
            for (const auto& city : individual) {
                cache_file << city << " ";
            }
            cache_file << endl; // 各個体の終わりに改行を追加
        }
        cache_file.close();
        cout << "Initial population saved to cache file." << endl;
        
    }
    
    
    cout << "Initial population created." << endl;
    
    // 終了判定関数
    // 世代数に達するか、最良の適応度が0.0025以上になったら終了
    struct {
        size_t generation = 0;
        size_t max_generations = generations;
        double max_fitness = 0.0025;
        
        bool operator()(const vector<Individual>& population, const vector<double>& fitness_values, const vector<vector<double>>& adjacency_matrix) {
            // // 最良の個体を見つける
            // auto best_fitness_ptr = std::max_element(fitness_values.begin(), fitness_values.end());
            // size_t best_index = std::distance(fitness_values.begin(), best_fitness_ptr);
            
            // // 100世代ごとに最良の個体を出力
            // // 最後の世代では必ず出力
            // if (generation % 100 == 0 || *best_fitness_ptr >= max_fitness ) {
            //     cout << "Generation " << generation << ": Best fitness = " << *best_fitness_ptr << endl;
            //     cout << "Best path: ";
            //     for (const auto& city : population[best_index]) {
            //         cout << city << " ";
            //     }
            //     cout << endl;
            // }
            
            double ave_fitness = 0.0;
            double max_fitness = 0.0;
            double min_fitness = std::numeric_limits<double>::max();
            for (const auto& fitness : fitness_values) {
                ave_fitness += fitness;
                max_fitness = std::max(max_fitness, fitness);
                min_fitness = std::min(min_fitness, fitness);
            }
            ave_fitness /= fitness_values.size();

            std::cout << "Generation " << generation << ": Average fitness = " << ave_fitness
                      << ", Max fitness = " << max_fitness
                      << ", Min fitness = " << min_fitness << std::endl;
            
            // 世代数を増やす
            ++generation;
            // 終了条件を満たすかどうかを判定
            return generation > max_generations;
        }
    } end_condition;
    // // 世代交代モデルSimpleGAを使用して、遺伝的アルゴリズムを実行
    // vector<Individual> result = mpi::genetic_algorithm::SimpleGA(population, end_condition, calc_fitness, edge_assembly_crossover, adjacency_matrix, rng);
    
    // 世代交代モデル ElitistRecombinationを使用して、遺伝的アルゴリズムを実行
    vector<Individual> result = mpi::genetic_algorithm::ElitistRecombination<10>(population, end_condition, calc_fitness, edge_assembly_crossover, adjacency_matrix, rng);

    // 最良の個体を見つける
    auto best_individual = std::max_element(result.begin(), result.end(), [&](const Individual& a, const Individual& b) {
        return calc_fitness(a, adjacency_matrix) < calc_fitness(b, adjacency_matrix);
    });
    
    // 最良の個体の適応度を出力
    cout << "Best fitness: " << calc_fitness(*best_individual, adjacency_matrix) << endl;
    cout << "Best path length: " << 1 / calc_fitness(*best_individual, adjacency_matrix) << endl;
    cout << "Best path: ";
    for (const auto& city : *best_individual) {
        cout << city << " ";
    }
    cout << endl;
    
    // // 遺伝的アルゴリズムのメインループ
    // for (size_t generation = 0; generation < generations; ++generation) {
    //     // 確率分布を作成
    //     vector<double> fitness_distribution(population_size);
    //     for (size_t i = 0; i < population_size; ++i) {
    //         fitness_distribution[i] = population[i].fitness;
    //     }
        
    //     discrete_distribution<size_t> dist(fitness_distribution.begin(), fitness_distribution.end());
        
    //     // 新しい集団を生成
    //     vector<Individual> new_population(population_size);
    //     for (size_t i = 0; i < population_size; ++i) {
    //         // 親を選択
    //         size_t parent1_index = dist(rng);
    //         size_t parent2_index = dist(rng);
            
    //         // 交叉
    //         new_population[i].path = edge_assembly_crossover(population[parent1_index].path, population[parent2_index].path, adjacency_matrix);
    //         // 適応度を計算
    //         new_population[i].fitness = calc_fitness(adjacency_matrix, new_population[i].path);
    //     }
        
    //     // 新しい集団を現在の集団に置き換え
    //     population = std::move(new_population);
    //     // 最良の個体を見つける
    //     auto best_individual = std::min_element(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
    //         return a.fitness < b.fitness;
    //     });
    //     // 最良の個体の適応度を出力
    //     if (generation % 100 == 0) {
    //         cout << "Generation " << generation << ": Best fitness = " << best_individual->fitness << endl;
    //         cout << "Best path: ";
    //         for (const auto& city : best_individual->path) {
    //             cout << city << " ";
    //         }
    //         cout << endl;
    //     } else if (best_individual->path[0] == best_individual->path[1]) {
    //         // print 
    //         cout << "Generation " << generation << ": Best fitness = " << best_individual->fitness << endl;
    //         cout << "Best path: ";
    //         for (const auto& city : best_individual->path) {
    //             cout << city << " ";
    //         }
    //         cout << endl;
    //     }
    // }

    
    return 0;
}