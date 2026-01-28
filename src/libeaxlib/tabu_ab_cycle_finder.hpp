#pragma once

#include <random>

#include "object_pool.hpp"
#include "limited_range_integer_set.hpp"

#include "eaxdef.hpp"
#include "object_pools.hpp"

namespace eax {

/**
 * @brief ABサイクルを見つける関数オブジェクトのクラス
 */
class TabuABCycleFinder {
public:
    TabuABCycleFinder(ObjectPools& object_pools)
        : any_size_vector_pool(object_pools.any_size_vector_pool.share()),
          vector_of_tsp_size_pool(object_pools.vector_of_tsp_size_pool.share()),
          doubly_linked_list_pool(object_pools.doubly_linked_list_pool.share()),
          LRIS_pool(object_pools.LRIS_pool.share()) {}

    TabuABCycleFinder(
        mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool,
        mpi::ObjectPool<std::vector<size_t>> vector_of_tsp_size_pool,
        mpi::ObjectPool<std::vector<std::array<size_t, 2>>> doubly_linked_list_pool,
        mpi::ObjectPool<mpi::LimitedRangeIntegerSet> LRIS_pool)
        : any_size_vector_pool(std::move(any_size_vector_pool)),
          vector_of_tsp_size_pool(std::move(vector_of_tsp_size_pool)),
          doubly_linked_list_pool(std::move(doubly_linked_list_pool)),
          LRIS_pool(std::move(LRIS_pool)) {}

    /**
     * @brief ABサイクルを見つける
     * @param needs 必要なABサイクルの数
     * @param parent1 親個体1
     * @param parent2 親個体2
     * @param rng 乱数生成器
     * @param tabu_edges タブーエッジの集合
     * @return ABサイクルのポインタのベクター
     */
    std::vector<mpi::pooled_unique_ptr<ab_cycle_t>> operator()(size_t needs,
            const doubly_linked_list_readable auto& parent1,
            const doubly_linked_list_readable auto& parent2,
            std::mt19937& rng,
            std::vector<std::pair<size_t, size_t>> const& tabu_edges)
    {
        using namespace std;
        const size_t city_count = parent1.size();

        auto parent1_tabu_ptr = doubly_linked_list_pool.acquire_unique();
        auto parent2_tabu_ptr = doubly_linked_list_pool.acquire_unique();
        std::vector<std::array<size_t, 2>>& parent1_tabu = *parent1_tabu_ptr;
        std::vector<std::array<size_t, 2>>& parent2_tabu = *parent2_tabu_ptr;
        parent1_tabu.assign(city_count, {numeric_limits<size_t>::max(), numeric_limits<size_t>::max()});
        parent2_tabu.assign(city_count, {numeric_limits<size_t>::max(), numeric_limits<size_t>::max()});
        for (const auto& [u, v] : tabu_edges) {
            // 親1のタブーエッジを設定
            if (parent1[u][0] == v) {
                parent1_tabu[u][0] = v;
            } else if (parent1[u][1] == v) {
                parent1_tabu[u][1] = v;
            }
            // 逆方向も設定
            if (parent1[v][0] == u) {
                parent1_tabu[v][0] = u;
                continue; // 親1のタブーエッジが見つかったら、親2の設定はスキップ (どちらかに存在すればtabuとして判定される)
            } else if (parent1[v][1] == u) {
                parent1_tabu[v][1] = u;
                continue;
            }
            
            // 親2のタブーエッジを設定
            if (parent2[u][0] == v) {
                parent2_tabu[u][0] = v;
            } else if (parent2[u][1] == v) {
                parent2_tabu[u][1] = v;
            }
            if (parent2[v][0] == u) {
                parent2_tabu[v][0] = u;
            } else if (parent2[v][1] == u) {
                parent2_tabu[v][1] = u;
            }
        }

        auto cities_having_2_edges_ptr = LRIS_pool.acquire_unique();
        auto cities_having_just_1_edge_ptr = LRIS_pool.acquire_unique();
        mpi::LimitedRangeIntegerSet& cities_having_2_edges = *cities_having_2_edges_ptr;
        mpi::LimitedRangeIntegerSet& cities_having_just_1_edge = *cities_having_just_1_edge_ptr;

        cities_having_2_edges.reset(mpi::LimitedRangeIntegerSet::InitSet::Universal);
        cities_having_just_1_edge.reset(mpi::LimitedRangeIntegerSet::InitSet::Empty);

        struct connections {
            // edge_pair[0]が least recently used edge
            // edge_pair[1]が最近通ったエッジ or すでにABサイクルを構成しているエッジ
            std::array<size_t, 2>& edge_pair;
            constexpr size_t least_recently_used_edge() const {
                return edge_pair[0];
            }

            constexpr size_t least_recently_used_edge_and_update() {
                std::swap(edge_pair[0], edge_pair[1]);
                return edge_pair[1];
            }

            constexpr size_t get_and_update(size_t index) {
                if (index == 0) {
                    std::swap(edge_pair[0], edge_pair[1]);
                }

                return edge_pair[1];
            }

            constexpr void update(size_t prev) {
                if (prev == edge_pair[0]) {
                    std::swap(edge_pair[0], edge_pair[1]);
                }
            }
        };

        auto parent1_copy_ptr = doubly_linked_list_pool.acquire_unique();
        auto parent2_copy_ptr = doubly_linked_list_pool.acquire_unique();
        std::vector<std::array<size_t, 2>>& parent1_copy = *parent1_copy_ptr;
        std::vector<std::array<size_t, 2>>& parent2_copy = *parent2_copy_ptr;
        for (size_t i = 0; i < city_count; ++i) {
            parent1_copy[i] = parent1[i];
            parent2_copy[i] = parent2[i];
        }

        struct parent {
            std::vector<std::array<size_t, 2>>& parent;
            constexpr connections operator[](size_t index) {
                return connections{parent[index]};
            }
        };

        struct {
            parent parent1;
            parent parent2;
            constexpr parent operator[](size_t index) {
                return index == 0 ? parent1 : parent2;
            }
        } parents = {{parent1_copy}, {parent2_copy}};

        vector<mpi::pooled_unique_ptr<ab_cycle_t>> AB_cycles;

        auto visited_ptr = any_size_vector_pool.acquire_unique();
        auto first_visited_ptr = vector_of_tsp_size_pool.acquire_unique();
        std::vector<size_t>& visited = *visited_ptr;
        std::vector<size_t>& first_visited = *first_visited_ptr;
        visited.clear();
        first_visited.assign(city_count, 0);

        // numeric_limits<size_t>::max() は、 cities_having_2_edges と
        // cities_having_just_1_edge のどちらにも含まれないので、
        // 以下のループの開始時に current_city は適切に初期化される
        size_t current_city = numeric_limits<size_t>::max();

        // どちらのエッジを通るか判断が必要な都市(2本のエッジを持つ都市)がなくなるまで
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

            auto cycle = find_AB_cycle_phase1(current_city, visited, first_visited, cities_having_2_edges, cities_having_just_1_edge, rng, parents);

            if (cycle->size() > 2 && !contains_tabu_edge(*cycle, parent1_tabu, parent2_tabu)) {
                AB_cycles.emplace_back(std::move(cycle));
                if (AB_cycles.size() >= needs) {
                    return AB_cycles; // 必要な数のABサイクルが見つかった
                }
            }
        }

        // 一つのエッジしか残っていないので、LRUをたどるだけでABサイクルを構成する
        while (cities_having_just_1_edge.size() > 0) {
            size_t start_city = *(cities_having_just_1_edge.begin());

            // 1つのエッジを持つ都市からABサイクルを構成する
            visited.clear();
            visited.push_back(start_city);

            size_t current_city = start_city;
            while (true) {
                size_t next_edge_parent = visited.size() % 2;
                size_t next_city = parents[next_edge_parent][current_city].least_recently_used_edge();
                visited.push_back(next_city);
                if (next_city == start_city) 
                    break;
                current_city = next_city;
            }

            auto cycle = create_AB_cycle(visited, 0, cities_having_2_edges, cities_having_just_1_edge);
            if (cycle->size() > 2 && !contains_tabu_edge(*cycle, parent1_tabu, parent2_tabu)) {
                AB_cycles.emplace_back(std::move(cycle));
                if (AB_cycles.size() >= needs) {
                    return AB_cycles; // 必要な数のABサイクルが見つかった
                }
            }

        }

        return AB_cycles;

    }

    using completeness_category = incomplete_ABCycleFinder_tag;
private:
    mpi::ObjectPool<std::vector<size_t>> any_size_vector_pool;
    mpi::ObjectPool<std::vector<size_t>> vector_of_tsp_size_pool;
    mpi::ObjectPool<std::vector<std::array<size_t, 2>>> doubly_linked_list_pool;
    mpi::ObjectPool<mpi::LimitedRangeIntegerSet> LRIS_pool;

    mpi::pooled_unique_ptr<ab_cycle_t> find_AB_cycle_phase1(size_t& current_city,
            std::vector<size_t>& visited,
            std::vector<size_t>& first_visited,
            mpi::LimitedRangeIntegerSet& cities_having_2_edges,
            mpi::LimitedRangeIntegerSet& cities_having_just_1_edge,
            std::mt19937& rng,
            auto& parents) {
        
        using namespace std;
        uniform_int_distribution<size_t> dist_01(0, 1);

        while (true) {  // ABサイクルを一つ見つけるまでループ
            size_t prev_city = current_city;
            size_t prev_first_visited = first_visited[prev_city];
            size_t prev_edge_count = cities_having_2_edges.contains(current_city) ? 2 : 1;
            size_t next_edge_parent = visited.size() % 2;

            if (prev_edge_count == 1) { // エッジが一つなら
                // 単純に最近最も使用されていないエッジを選択
                // もうほかの枝は存在しないので LRU は更新しない
                current_city = parents[next_edge_parent][prev_city].least_recently_used_edge();
            } else {
                if (prev_first_visited != visited.size() - 1) {
                    // 2回訪れた都市なら、残り一つのエッジを選択
                    // prev_first_visitedが直前じゃなければ、2回訪れたことが分かる
                    current_city = parents[next_edge_parent][prev_city].least_recently_used_edge_and_update();
                } else { // そうじゃないなら
                    current_city = parents[next_edge_parent][prev_city].get_and_update(dist_01(rng));
                }
            }

            visited.push_back(current_city);

            size_t current_edge_count = cities_having_2_edges.contains(current_city) ? 2 : 1;
            if (current_edge_count == 1) {
                // エッジの数が1なら、探索途中か、スタートにたどり着いて一周したか
                if (current_city == visited.front()) { // スタートにたどり着いたなら
                    // ABサイクル構成処理
                    return create_AB_cycle(visited, 0, cities_having_2_edges, cities_having_just_1_edge);
                } else {
                    continue; // 探索途中なら、次の都市へ
                }
            } else {
                // エッジの数が2なら、探索途中か、スタートにたどり着いて一周したか、途中で交差してABサイクルを構成するか
                // 次の都市のLRUを更新
                parents[next_edge_parent][current_city].update(prev_city);
                if (current_city == visited.front()) { // スタートにたどりついたなら
                    if ((visited.size() + 1) % 2 == 0) { // 親Bのエッジでスタートして、親Aのエッジで戻ってきた
                        // ABサイクル構成処理
                        return create_AB_cycle(visited, 0, cities_having_2_edges, cities_having_just_1_edge);
                    } else { // 親Bのエッジでスタートして、親Bのエッジで帰ってきた
                        continue;
                    }
                } else if (first_visited[current_city] != 0 && (visited.size() - first_visited[current_city] + 1) % 2 == 0) {
                    // 交差している　かつ　ABサイクルを構成するなら
                    return create_AB_cycle(visited, first_visited[current_city], cities_having_2_edges, cities_having_just_1_edge);
                } else if (first_visited[current_city] != 0) { // 交差しているが、ABサイクルを構成しないなら
                    continue;
                } else { // 初めて通る都市なら
                    first_visited[current_city] = visited.size() - 1;
                    continue;
                }
            }
        }

    }

    mpi::pooled_unique_ptr<ab_cycle_t> create_AB_cycle(std::vector<size_t>& finding_path,
                        size_t end_index,
                        mpi::LimitedRangeIntegerSet& cities_having_2_edges,
                        mpi::LimitedRangeIntegerSet& cities_having_just_1_edge)
    {
        bool starts_with_B = finding_path.size() % 2 == 0;
        size_t start_index = finding_path.size() - 1;
        auto AB_cycle_ptr = any_size_vector_pool.acquire_unique();
        std::vector<size_t>& AB_cycle = *AB_cycle_ptr;
        AB_cycle.clear();
        AB_cycle.reserve(start_index - end_index);
        
        size_t last = 0;
        // ABサイクルはAのエッジから始まるようにする
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

        return AB_cycle_ptr;
    }

    /**
     * @brief ABサイクルがtabuエッジを含むかどうかを判定する
     * @param AB_cycle ABサイクル
     * @param parent1_tabu 親1のタブーエッジ
     * @param parent2_tabu 親2のタブーエッジ
     * @return 含む場合はtrue、含まない場合はfalse
     */
    static bool contains_tabu_edge(const ab_cycle_t& AB_cycle,
            const std::vector<std::array<size_t, 2>>& parent1_tabu,
            const std::vector<std::array<size_t, 2>>& parent2_tabu)
    {
        for (size_t i = 0; i < AB_cycle.size(); ++i) {
            size_t u = AB_cycle[i];
            size_t v = AB_cycle[(i + 1) % AB_cycle.size()];

            if (parent1_tabu[u][0] == v || parent1_tabu[u][1] == v ||
                parent2_tabu[u][0] == v || parent2_tabu[u][1] == v) {
                return true;
            }
        }

        return false;
    }
};
}