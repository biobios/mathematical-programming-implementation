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

// 乱数生成器はグローバルに定義
std::mt19937 rng;

double calc_fitness(const std::vector<std::vector<double>>& adjacency_matrix, std::vector<size_t>& path){
    double fitness = 0.0;
    for (size_t i = 0; i < path.size() - 1; ++i) {
        fitness += adjacency_matrix[path[i]][path[i + 1]];
    }
    // 最後の都市から最初の都市への距離を加算
    fitness += adjacency_matrix[path.back()][path.front()];
    return 1 / fitness;
}

std::vector<size_t> edge_assembly_crossover(const std::vector<size_t>& parent1, const std::vector<size_t>& parent2, 
                                            const std::vector<std::vector<double>>& adjacency_matrix) {
    struct edge {
        size_t from;
        size_t to;
        bool is_parent1;
        bool operator<(const edge& other) const {
            return std::tie(from, to, is_parent1) < std::tie(other.from, other.to, other.is_parent1);
        }
    };
    
    using namespace std;

    vector<set<edge>> AB_cycles;
    
    map<size_t, size_t> adjacency_list_parent1_from;
    map<size_t, size_t> adjacency_list_parent2_from;
    map<size_t, size_t> adjacency_list_parent1_to;
    map<size_t, size_t> adjacency_list_parent2_to;

    
    for (size_t i = 0; i < parent1.size(); ++i) {
        size_t next_i = (i + 1) % parent1.size();
        
        // 隣接リストに追加
        adjacency_list_parent1_from[parent1[i]] = parent1[next_i];
        adjacency_list_parent1_to[parent1[next_i]] = parent1[i];
        adjacency_list_parent2_from[parent2[i]] = parent2[next_i];
        adjacency_list_parent2_to[parent2[next_i]] = parent2[i];
    }
    // 子供用にコピーしておく
    map<size_t, size_t> child_adjacency_list_from = adjacency_list_parent1_from;
    
    // すべてのmapの要素数は同じであるので、一つ確かめるだけでよい
    while (!adjacency_list_parent1_from.empty()) {
        set<edge> cycle;
        size_t current_city = adjacency_list_parent1_from.begin()->first;
        
        while(adjacency_list_parent1_from.contains(current_city)) {
            // リストから現在のエッジを削除
            auto current_edge = *adjacency_list_parent1_from.find(current_city);
            adjacency_list_parent1_from.erase(current_edge.first);
            adjacency_list_parent1_to.erase(current_edge.second);
            
            cycle.insert({current_edge.first, current_edge.second, true});
            
            // 対応するエッジを親2から探す
            auto corresponding_edge = *adjacency_list_parent2_to.find(current_edge.second);
            // 親2のエッジを削除
            adjacency_list_parent2_from.erase(corresponding_edge.second);
            adjacency_list_parent2_to.erase(corresponding_edge.first);
            
            cycle.insert({corresponding_edge.second, corresponding_edge.first, false});

            current_city = corresponding_edge.second;
        }
        
        // サイズが２以下のサイクルは無視
        if (cycle.size() > 2)
            AB_cycles.push_back(move(cycle));
    }
    // 1個以上N-1個以下のABサイクルをランダムに選択して、E-setを作成
    // もしAB_cyclesが１個なら、parent1かparent2をランダムに返す
    if (AB_cycles.size() < 2) {
        return (rng() % 2 == 0) ? parent1 : parent2;
    }
    set<edge> E_set;
    vector<size_t> AB_cycle_indices(AB_cycles.size());
    std::iota(AB_cycle_indices.begin(), AB_cycle_indices.end(), 0); // 0からN-1までの整数を初期化
    std::shuffle(AB_cycle_indices.begin(), AB_cycle_indices.end(), rng);
    std::uniform_int_distribution<size_t> dist(1, AB_cycles.size() - 1);
    size_t num_cycles = dist(rng);
    for (size_t i = 0; i < num_cycles; ++i) {
        E_set.merge(AB_cycles[AB_cycle_indices[i]]);
    }
    
    // parent1の隣接リストをコピーして、E-setを適用
    vector<size_t> child_path(parent1.size());
    for (const auto [start, end, is_parent1] : E_set) {
        if (is_parent1) continue;
        // parent2のエッジで上書き
        child_adjacency_list_from[start] = end;
    }
    
    // 緩和個体が作成できたので、修正操作を行う
    vector<set<edge>> partial_cycles;
    while (!child_adjacency_list_from.empty()) {
        set<edge> cycle;
        size_t current_city = child_adjacency_list_from.begin()->first;

        while(child_adjacency_list_from.contains(current_city)) {
            auto next_city = child_adjacency_list_from[current_city];
            // リストから現在のエッジを削除
            child_adjacency_list_from.erase(current_city);
            cycle.insert({current_city, next_city, true});
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
        double min_weight = std::numeric_limits<double>::max();
        size_t min_cost_index = 0;

        set<edge>& min_cycle = partial_cycles[min_cycle_index];
        for (size_t i = 0; i < partial_cycles.size(); ++i) {
            if (i == min_cycle_index) continue;
            
            for (const auto& edge1 : partial_cycles[min_cycle_index]) {
                for (const auto& edge2 : partial_cycles[i]) {
                    double cost = - adjacency_matrix[edge1.from][edge1.to] - adjacency_matrix[edge2.from][edge2.to]
                                  + adjacency_matrix[edge1.from][edge2.to] + adjacency_matrix[edge2.from][edge1.to];
                    
                    if (cost < min_weight) {
                        min_weight = cost;
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
        partial_cycles[min_cycle_index].insert({e1.from, e2.to, true});
        partial_cycles[min_cycle_index].insert({e2.from, e1.to, true});
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
    // そして、child_pathに変換
    size_t current_city = final_adjacency_list.front();
    for (size_t i = 0; i < child_path.size(); ++i) {
        child_path[i] = current_city;
        current_city = final_adjacency_list[current_city];
    }
    
    return child_path;
}

int main()
{
    using namespace std;
    std::ifstream file("city_position.csv");
    
    if (!file.is_open()) {
        cerr << "Error: Could not open the file 'city_position.csv'." << endl;
        return 1;
    }
    vector<tuple<int/*city id*/, double/*x*/, double/*y*/>> city_positions;
    string line;
    // ヘッダ行を読み飛ばす
    getline(file, line);
    
    while (getline(file, line)) {
        int id;
        double x, y;
        char comma; // コンマを読み飛ばすための変数
        istringstream iss(line);
        
        if (!(iss >> id >> comma >> x >> comma >> y)) {
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
            // ユークリッド距離を計算
            double distance = sqrt((x_i - x_j) * (x_i - x_j) + (y_i - y_j) * (y_i - y_j));
            
            adjacency_matrix[i][j] = distance;
            adjacency_matrix[j][i] = distance;
        }
    }
    
    cout << "Number of cities: " << city_positions.size() << endl;
    
    // 集団サイズ
    constexpr size_t population_size = 100;
    // 世代数
    constexpr size_t generations = 10000;
    
    struct Individual {
        vector<size_t> path;
        double fitness = 0.0; // 適応度
    };
    
    // 集団を初期化
    vector<Individual> population(population_size);
    
    for (size_t i = 0; i < population_size; ++i) {
        population[i].path.resize(city_positions.size());
        iota(population[i].path.begin(), population[i].path.end(), 0); // 0からN-1までの整数を初期化
        // ランダムにシャッフル
        shuffle(population[i].path.begin(), population[i].path.end(), rng);
        
        // 適応度を計算
        population[i].fitness = calc_fitness(adjacency_matrix, population[i].path);
    }
    
    cout << "Initial population created." << endl;
    
    // 遺伝的アルゴリズムのメインループ
    for (size_t generation = 0; generation < generations; ++generation) {
        // 確率分布を作成
        vector<double> fitness_distribution(population_size);
        for (size_t i = 0; i < population_size; ++i) {
            fitness_distribution[i] = population[i].fitness;
        }
        
        discrete_distribution<size_t> dist(fitness_distribution.begin(), fitness_distribution.end());
        
        // 新しい集団を生成
        vector<Individual> new_population(population_size);
        for (size_t i = 0; i < population_size; ++i) {
            // 親を選択
            size_t parent1_index = dist(rng);
            size_t parent2_index = dist(rng);
            
            // 交叉
            new_population[i].path = edge_assembly_crossover(population[parent1_index].path, population[parent2_index].path, adjacency_matrix);
            // 適応度を計算
            new_population[i].fitness = calc_fitness(adjacency_matrix, new_population[i].path);
        }
        
        // 新しい集団を現在の集団に置き換え
        population = std::move(new_population);
        // 最良の個体を見つける
        auto best_individual = std::min_element(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
            return a.fitness < b.fitness;
        });
        // 最良の個体の適応度を出力
        if (generation % 100 == 0) {
            cout << "Generation " << generation << ": Best fitness = " << best_individual->fitness << endl;
            cout << "Best path: ";
            for (const auto& city : best_individual->path) {
                cout << city << " ";
            }
            cout << endl;
        } else if (best_individual->path[0] == best_individual->path[1]) {
            // print 
            cout << "Generation " << generation << ": Best fitness = " << best_individual->fitness << endl;
            cout << "Best path: ";
            for (const auto& city : best_individual->path) {
                cout << city << " ";
            }
            cout << endl;
        }
    }

    
    return 0;
}