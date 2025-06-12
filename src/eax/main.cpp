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
    struct edge {
        size_t from;
        size_t to;
        // setに格納するための比較演算子
        bool operator<(const edge& other) const {
            return std::tie(from, to) < std::tie(other.from, other.to);
        }
    };
    
    using namespace std;

    // parent2のエッジのみを含むABサイクルの配列
    vector<set<edge>> AB_cycles_only_from_parent2;
    
    // parent1の始点をキー、終点を値とする隣接リスト
    map<size_t, size_t> adjacency_list_parent1_from;
    // parent2の終点をキー、始点を値とする隣接リスト
    map<size_t, size_t> adjacency_list_parent2_to;

    
    // parent1とparent2のエッジを隣接リストに変換
    for (size_t i = 0; i < parent1.size(); ++i) {
        size_t next_i = (i + 1) % parent1.size();
        
        adjacency_list_parent1_from[parent1[i]] = parent1[next_i];
        adjacency_list_parent2_to[parent2[next_i]] = parent2[i];
    }

    // 子供用に隣接リストをコピーしておく
    map<size_t, size_t> base_of_child_adjacency_list_from = adjacency_list_parent1_from;
    
    // ABサイクルに分割する
    // すべてのmapの要素数は同じであるので、一つが空であるか確かめるだけでよい
    while (!adjacency_list_parent1_from.empty()) {
        set<edge> cycle;
        size_t current_city = adjacency_list_parent1_from.begin()->first;
        
        while(adjacency_list_parent1_from.contains(current_city)) {
            // リストから現在のエッジを削除
            auto current_edge = *adjacency_list_parent1_from.find(current_city);
            adjacency_list_parent1_from.erase(current_edge.first);
            
            // 対応するエッジを親2から探す
            auto corresponding_edge = *adjacency_list_parent2_to.find(current_edge.second);
            // 親2のエッジを削除
            adjacency_list_parent2_to.erase(corresponding_edge.first);
            
            cycle.insert({corresponding_edge.second, corresponding_edge.first});

            current_city = corresponding_edge.second;
        }
        
        // サイズが２以下のサイクルは無視(無効ABサイクル)
        if (cycle.size() > 2)
            AB_cycles_only_from_parent2.push_back(move(cycle));
    }
    
    vector<vector<size_t>> children(children_size);

    for (size_t children_count = 0; children_count < children_size; ++children_count) {
        vector<size_t>& child_path = children[children_count];
        // もしAB_cyclesが１個なら、parent1かparent2をランダムに返す 
        if (AB_cycles_only_from_parent2.size() < 2) {
            child_path = (rng() % 2 == 0) ? parent1 : parent2;
            continue;
        }

        // 1個以上N-1個以下のABサイクルをランダムに選択して、E-setを作成
        set<edge> E_set;

        vector<size_t> AB_cycle_indices(AB_cycles_only_from_parent2.size());
        std::iota(AB_cycle_indices.begin(), AB_cycle_indices.end(), 0); // 0からN-1までの整数を初期化
        std::shuffle(AB_cycle_indices.begin(), AB_cycle_indices.end(), rng); // ランダムにシャッフル

        // 使用するABサイクルの数をランダムに決定(1以上N-1以下)
        std::uniform_int_distribution<size_t> dist(1, AB_cycles_only_from_parent2.size() - 1);
        size_t num_cycles = dist(rng);

        for (size_t i = 0; i < num_cycles; ++i) {
            E_set.merge(AB_cycles_only_from_parent2[AB_cycle_indices[i]]);
        }
        
        // parent1の隣接リストをコピーしたchild_adjacency_list_fromに、E-setを適用
        map<size_t, size_t> child_adjacency_list_from = base_of_child_adjacency_list_from;
        for (const auto e : E_set) {
            // parent2のエッジで上書き
            child_adjacency_list_from[e.from] = e.to;
        }
        
        // 緩和個体が作成できたので、修正操作を行う
        // 隣接リストから部分巡回路の配列に変換
        vector<set<edge>> partial_cycles;
        while (!child_adjacency_list_from.empty()) {
            set<edge> cycle;
            size_t current_city = child_adjacency_list_from.begin()->first;

            while(child_adjacency_list_from.contains(current_city)) {
                auto next_city = child_adjacency_list_from[current_city];
                // リストから現在のエッジを削除
                child_adjacency_list_from.erase(current_city);
                cycle.insert({current_city, next_city});
                current_city = next_city;
            }

            partial_cycles.push_back(move(cycle));
            
        }
        
        // 要素数が最小の部分巡回路を定める
        // その部分巡回路と接続するときのコストが最小の部分巡回路を見つけ、接続する
        // 部分巡回路のサイズが１になるまで繰り返す
        
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
            double min_cost = std::numeric_limits<double>::max();
            size_t min_cost_index = 0;

            set<edge>& min_cycle = partial_cycles[min_cycle_index];
            for (size_t i = 0; i < partial_cycles.size(); ++i) {
                if (i == min_cycle_index) continue;
                
                for (const auto& edge1 : partial_cycles[min_cycle_index]) {
                    for (const auto& edge2 : partial_cycles[i]) {
                        double cost = - adjacency_matrix[edge1.from][edge1.to] - adjacency_matrix[edge2.from][edge2.to]
                                    + adjacency_matrix[edge1.from][edge2.to] + adjacency_matrix[edge2.from][edge1.to];
                        
                        if (cost < min_cost) {
                            min_cost = cost;
                            e1 = edge1;
                            e2 = edge2;
                            min_cost_index = i;
                        }
                    }
                }
            }
            
            // e1とe2をそれぞれ削除して、接続する辺をpartial_cycles[min_cycle_index]に追加
            partial_cycles[min_cycle_index].erase(e1);
            partial_cycles[min_cost_index].erase(e2);
            partial_cycles[min_cycle_index].insert({e1.from, e2.to});
            partial_cycles[min_cycle_index].insert({e2.from, e1.to});
            // partial_cycles[min_cost_index]をmin_cycle_indexにマージ
            partial_cycles[min_cycle_index].merge(partial_cycles[min_cost_index]);

            // 末尾の部分巡回路をmin_cost_indexに移動して、削除
            if (min_cost_index < partial_cycles.size() - 1) {
                std::swap(partial_cycles[min_cost_index], partial_cycles.back());
            }
            partial_cycles.pop_back();
        }
        
        // 最後に残った部分巡回路をchild_pathに変換
        const set<edge>& final_cycle = partial_cycles.front();
        // まず、隣接リストに変換
        vector<size_t> final_adjacency_list(final_cycle.size());
        for (const auto& e : final_cycle) {
            final_adjacency_list[e.from] = e.to;
        }
        // そして、child_path(パス表現)に変換
        size_t current_city = final_adjacency_list.front();
        child_path.resize(final_adjacency_list.size());
        for (size_t i = 0; i < child_path.size(); ++i) {
            child_path[i] = current_city;
            current_city = final_adjacency_list[current_city];
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
    // std::ifstream file("city_position.csv");
    
    // if (!file.is_open()) {
    //     cerr << "Error: Could not open the file 'city_position.csv'." << endl;
    //     return 1;
    // }
    // vector<tuple<int/*city id*/, double/*x*/, double/*y*/>> city_positions;
    // string line;
    // // ヘッダ行を読み飛ばす
    // getline(file, line);
    
    // while (getline(file, line)) {
    //     int id;
    //     double x, y;
    //     char comma; // コンマを読み飛ばすための変数
    //     istringstream iss(line);
        
    //     if (!(iss >> id >> comma >> x >> comma >> y)) {
    //         cerr << "Error: Invalid line format: " << line << endl;
    //         continue; // 無効な行はスキップ
    //     }
        
    //     city_positions.emplace_back(id, x, y);
    // }
    // file.close();
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
            // 最良の個体を出力
            auto best_fitness_ptr = std::max_element(fitness_values.begin(), fitness_values.end());
            size_t best_index = std::distance(fitness_values.begin(), best_fitness_ptr);
            cout << "Best fitness: " << *best_fitness_ptr << endl;
            cout << "Best path length: " << 1 / *best_fitness_ptr << endl;
            cout << "Best path: ";
            for (const auto& city : population[best_index]) {
                cout << city << " ";
            }
            cout << endl;
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