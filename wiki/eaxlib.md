# eaxlib (EAX Library)

## 概要 / Overview

eaxlibは、EAX (Edge Assembly Crossover) アルゴリズムを実装するためのユーティリティライブラリです。AB-cycle構築、E-set管理、オブジェクトプールなど、EAXアルゴリズムの実装に必要な基本機能を提供します。

eaxlib is a utility library for implementing the EAX (Edge Assembly Crossover) algorithm. It provides basic functions needed for EAX algorithm implementation, such as AB-cycle construction, E-set management, and object pools.

## ディレクトリ / Directory

```
src/libeaxlib/
```

## ビルド / Build

```bash
# 通常ビルド / Normal build
make bin/libeaxlib.a

# デバッグビルド / Debug build
make bin/debug/libeaxlib.a

# プロファイリングビルド / Profiling build
make bin/prof/libeaxlib.a
```

## 主要コンポーネント / Main Components

### 1. EAX 交叉オペレータ / EAX Crossover Operator

#### EAX Normal

標準的なEAX交叉を実装。

Implements standard EAX crossover.

```cpp
#include "eax_normal.hpp"

// EAX交叉オペレータを作成 / Create EAX crossover operator
mpi::eax::EAXNormal eax_operator(distance_matrix);

// 2つの親から子を生成 / Generate offspring from two parents
auto offspring = eax_operator.crossover(parent1, parent2);
```

#### EAX N-AB

N個のAB-cycleを使用する拡張版EAX。

Extended EAX using N AB-cycles.

```cpp
#include "eax_n_ab.hpp"

// N-AB EAX交叉オペレータを作成 / Create N-AB EAX crossover operator
mpi::eax::EAXNAb eax_n_ab_operator(distance_matrix, num_ab_cycles);

// 交叉を実行 / Perform crossover
auto offspring = eax_n_ab_operator.crossover(parent1, parent2);
```

### 2. AB-cycle 構築 / AB-cycle Construction

AB-cycleは、2つの親のツアーからエッジを交互に選択して形成される閉路です。

An AB-cycle is a cycle formed by alternately selecting edges from the tours of two parents.

```cpp
#include "ab_cycle_builder.hpp"

// AB-cycle構築器を作成 / Create AB-cycle builder
mpi::eax::ABCycleBuilder builder;

// AB-cycleを構築 / Build AB-cycles
auto ab_cycles = builder.build(parent1, parent2, distance_matrix);

// AB-cycleの情報を取得 / Get AB-cycle information
for(const auto& cycle : ab_cycles) {
    std::cout << "Cycle length: " << cycle.length() << std::endl;
    std::cout << "Cycle edges: ";
    for(const auto& edge : cycle.edges()) {
        std::cout << "(" << edge.from << "," << edge.to << ") ";
    }
    std::cout << std::endl;
}
```

### 3. E-set 管理 / E-set Management

E-setは、AB-cycleから選択されたエッジの集合です。

An E-set is a set of edges selected from AB-cycles.

```cpp
#include "e_set.hpp"

// E-setを作成 / Create E-set
mpi::eax::ESet e_set;

// エッジを追加 / Add edges
e_set.add_edge(edge1);
e_set.add_edge(edge2);

// E-setを適用して新しいツアーを生成 / Apply E-set to generate new tour
auto new_tour = e_set.apply(parent_tour);

// E-setのサイズを取得 / Get E-set size
std::size_t size = e_set.size();
```

### 4. 中間個体 / Intermediate Individual

交叉中に使用される中間的なツアー表現。

Intermediate tour representation used during crossover.

```cpp
#include "intermediate_individual.hpp"

// 中間個体を作成 / Create intermediate individual
mpi::eax::IntermediateIndividual intermediate(num_cities);

// エッジを追加 / Add edges
intermediate.add_edge(from, to);

// ツアーに変換 / Convert to tour
auto tour = intermediate.to_tour();

// 有効性を確認 / Check validity
bool is_valid = intermediate.is_valid_tour();
```

### 5. Block2 E-set アセンブラー / Block2 E-set Assembler

効率的なE-set構築のためのアセンブラー。

Assembler for efficient E-set construction.

```cpp
#include "block2_e_set_assembler.hpp"

// アセンブラーを作成 / Create assembler
mpi::eax::Block2ESetAssembler assembler;

// E-setを組み立て / Assemble E-set
auto e_sets = assembler.assemble(ab_cycles, parent1, parent2);

// 各E-setを評価 / Evaluate each E-set
for(const auto& e_set : e_sets) {
    auto offspring = e_set.apply(parent1);
    double fitness = evaluate(offspring);
    std::cout << "Fitness: " << fitness << std::endl;
}
```

### 6. サブツアーリスト / Subtour List

サブツアーの管理と検出。

Subtour management and detection.

```cpp
#include "subtour_list.hpp"

// サブツアーリストを作成 / Create subtour list
mpi::eax::SubtourList subtours(num_cities);

// サブツアーを検出 / Detect subtours
subtours.detect(tour_edges);

// サブツアーの数を取得 / Get number of subtours
std::size_t num_subtours = subtours.count();

// 各サブツアーを処理 / Process each subtour
for(std::size_t i = 0; i < num_subtours; ++i) {
    auto subtour = subtours.get(i);
    std::cout << "Subtour " << i << " size: " << subtour.size() << std::endl;
}
```

### 7. オブジェクトプール / Object Pools

メモリ割り当てを最適化するためのオブジェクトプール。

Object pools for optimizing memory allocation.

```cpp
#include "object_pools.hpp"

// オブジェクトプールを初期化 / Initialize object pool
mpi::eax::ObjectPools pools(max_num_cities);

// オブジェクトを取得 / Get object from pool
auto* obj = pools.acquire<SomeObject>();

// オブジェクトを使用 / Use object
obj->do_something();

// オブジェクトを返却 / Return object to pool
pools.release(obj);

// プールをクリア / Clear pool
pools.clear();
```

### 8. EAX 定義 / EAX Definitions

共通の定義と型。

Common definitions and types.

```cpp
#include "eaxdef.hpp"

// エッジの表現 / Edge representation
mpi::eax::Edge edge{from_city, to_city};

// ツアーの表現（隣接リスト形式） / Tour representation (adjacency list format)
mpi::eax::Tour tour(num_cities);

// 距離行列の型 / Distance matrix type
using DistanceMatrix = std::vector<std::vector<int64_t>>;
```

### 9. EAX ランダム / EAX Random

EAX用の乱数生成ユーティリティ。

Random number generation utilities for EAX.

```cpp
#include "eax_rand.hpp"

// 乱数生成器を初期化 / Initialize random generator
mpi::eax::init_random(seed);

// ランダムな整数を生成 / Generate random integer
int random_int = mpi::eax::random_int(0, max_value);

// ランダムな実数を生成 / Generate random real
double random_real = mpi::eax::random_real(0.0, 1.0);

// ランダムな順列を生成 / Generate random permutation
std::vector<int> permutation = mpi::eax::random_permutation(n);
```

## EAX アルゴリズムの流れ / EAX Algorithm Flow

### 1. 準備フェーズ / Preparation Phase

```cpp
// 距離行列を準備 / Prepare distance matrix
DistanceMatrix dist_matrix = load_distance_matrix("problem.tsp");

// オブジェクトプールを初期化 / Initialize object pools
ObjectPools pools(num_cities);

// EAX交叉オペレータを作成 / Create EAX crossover operator
EAXNormal eax(dist_matrix);
```

### 2. 交叉フェーズ / Crossover Phase

```cpp
// 親を選択 / Select parents
Tour parent1 = select_parent();
Tour parent2 = select_parent();

// AB-cycleを構築 / Build AB-cycles
ABCycleBuilder builder;
auto ab_cycles = builder.build(parent1, parent2, dist_matrix);

// E-setを生成 / Generate E-sets
Block2ESetAssembler assembler;
auto e_sets = assembler.assemble(ab_cycles, parent1, parent2);

// 各E-setから子を生成 / Generate offspring from each E-set
std::vector<Tour> offspring_list;
for(const auto& e_set : e_sets) {
    IntermediateIndividual intermediate(num_cities);
    
    // E-setを適用 / Apply E-set
    for(const auto& edge : parent1.edges()) {
        if(!e_set.contains(edge)) {
            intermediate.add_edge(edge);
        }
    }
    
    for(const auto& edge : parent2.edges()) {
        if(e_set.contains(edge)) {
            intermediate.add_edge(edge);
        }
    }
    
    // ツアーを完成 / Complete tour
    if(intermediate.is_valid_tour()) {
        offspring_list.push_back(intermediate.to_tour());
    }
}

// 最良の子を選択 / Select best offspring
Tour best_offspring = select_best(offspring_list);
```

### 3. 検証フェーズ / Validation Phase

```cpp
// サブツアーを検出 / Detect subtours
SubtourList subtours(num_cities);
subtours.detect(offspring.edges());

if(subtours.count() > 1) {
    // サブツアーがある場合は修正 / Fix if subtours exist
    offspring = repair_subtours(offspring, subtours);
}

// ツアーの有効性を確認 / Verify tour validity
assert(is_valid_tsp_tour(offspring));
```

## 最適化技術 / Optimization Techniques

### 1. オブジェクトプール / Object Pooling

頻繁に生成・破棄されるオブジェクトを再利用:

Reuse frequently created and destroyed objects:

- **メモリアロケーションの削減 / Reduced Memory Allocation**: `new`/`delete`の呼び出しを削減 / Reduce calls to `new`/`delete`
- **キャッシュ効率 / Cache Efficiency**: 局所性を改善 / Improve locality
- **パフォーマンス向上 / Performance Improvement**: 10-30%の高速化 / 10-30% speedup

### 2. インライン化 / Inlining

小さな関数はすべてインライン化:

Inline all small functions:

```cpp
inline void add_edge(int from, int to) {
    // 実装 / Implementation
}
```

### 3. メモリレイアウト / Memory Layout

キャッシュフレンドリーなデータ構造:

Cache-friendly data structures:

```cpp
// 悪い例: ポインタの配列 / Bad: array of pointers
std::vector<Tour*> tours;

// 良い例: 連続メモリ / Good: contiguous memory
std::vector<Tour> tours;
```

### 4. 事前計算 / Pre-computation

頻繁に使用される値を事前計算:

Pre-compute frequently used values:

```cpp
// 距離の事前計算 / Pre-compute distances
std::vector<std::vector<int64_t>> dist_matrix = 
    compute_all_distances(cities);

// AB-cycleの事前構築 / Pre-build AB-cycles
auto ab_cycles = pre_build_ab_cycles(population);
```

## データ構造 / Data Structures

### ツアーの表現 / Tour Representation

#### 隣接リスト形式 / Adjacency List Format

```cpp
// 各都市の次の都市を格納 / Store next city for each city
std::vector<std::array<int, 2>> adjacency_list(num_cities);

// 例: 都市0の隣接都市は1と5 / Example: city 0 is adjacent to 1 and 5
adjacency_list[0] = {1, 5};
```

利点:
- エッジの追加/削除が高速 / Fast edge addition/removal
- AB-cycle構築に適している / Suitable for AB-cycle construction

#### 経路形式 / Path Format

```cpp
// 訪問順序を格納 / Store visit order
std::vector<int> path = {0, 3, 5, 2, 1, 4, 0};
```

利点:
- 直感的 / Intuitive
- 距離計算が簡単 / Easy distance calculation

### エッジの表現 / Edge Representation

```cpp
struct Edge {
    int from;
    int to;
    
    bool operator==(const Edge& other) const {
        return (from == other.from && to == other.to) ||
               (from == other.to && to == other.from);
    }
};
```

## パフォーマンスベンチマーク / Performance Benchmarks

### AB-cycle構築 / AB-cycle Construction

| 都市数 / Cities | 時間 (ms) / Time (ms) | メモリ (MB) / Memory (MB) |
|----------------|----------------------|--------------------------|
| 100 | 5 | 2 |
| 300 | 18 | 8 |
| 500 | 35 | 15 |
| 1000 | 85 | 35 |

### E-set 組み立て / E-set Assembly

| AB-cycles数 / # of AB-cycles | 時間 (ms) / Time (ms) |
|------------------------------|----------------------|
| 10 | 2 |
| 50 | 12 |
| 100 | 28 |
| 200 | 65 |

## 使用例 / Usage Examples

### 完全な交叉例 / Complete Crossover Example

```cpp
#include "eaxlib.hpp"

int main() {
    // TSPインスタンスを読み込み / Load TSP instance
    auto tsp_data = load_tsp("problem.tsp");
    int num_cities = tsp_data.dimension;
    auto dist_matrix = tsp_data.distance_matrix;
    
    // オブジェクトプールを初期化 / Initialize object pools
    mpi::eax::ObjectPools pools(num_cities);
    
    // EAX交叉オペレータを作成 / Create EAX crossover operator
    mpi::eax::EAXNormal eax(dist_matrix);
    
    // 親を作成 / Create parents
    Tour parent1 = create_random_tour(num_cities);
    Tour parent2 = create_random_tour(num_cities);
    
    // 交叉を実行 / Perform crossover
    auto offspring = eax.crossover(parent1, parent2);
    
    // 結果を評価 / Evaluate result
    double fitness = calculate_tour_length(offspring, dist_matrix);
    std::cout << "Offspring fitness: " << fitness << std::endl;
    
    return 0;
}
```

### カスタムE-set選択 / Custom E-set Selection

```cpp
// カスタムE-set選択戦略 / Custom E-set selection strategy
class CustomESetSelector {
public:
    ESet select(const std::vector<ABCycle>& ab_cycles) {
        ESet e_set;
        
        // カスタムロジック: 短いAB-cycleを優先 / Custom logic: prefer short AB-cycles
        std::vector<ABCycle> sorted_cycles = ab_cycles;
        std::sort(sorted_cycles.begin(), sorted_cycles.end(),
                  [](const ABCycle& a, const ABCycle& b) {
                      return a.length() < b.length();
                  });
        
        // 最初の k 個のAB-cycleを選択 / Select first k AB-cycles
        for(int i = 0; i < std::min(k, (int)sorted_cycles.size()); ++i) {
            for(const auto& edge : sorted_cycles[i].edges()) {
                e_set.add_edge(edge);
            }
        }
        
        return e_set;
    }
    
private:
    int k = 5;  // 選択するAB-cycleの数 / Number of AB-cycles to select
};
```

## トラブルシューティング / Troubleshooting

### 問題: サブツアーが発生する / Problem: Subtours Occur

**原因 / Cause**: E-setの選択が不適切 / Improper E-set selection

**解決策 / Solution**:
```cpp
// サブツアーを検出して修正 / Detect and fix subtours
SubtourList subtours(num_cities);
subtours.detect(tour.edges());

if(subtours.count() > 1) {
    tour = repair_subtours(tour, subtours);
}
```

### 問題: メモリリーク / Problem: Memory Leak

**原因 / Cause**: オブジェクトプールから取得したオブジェクトを返却していない / Not returning objects acquired from object pool

**解決策 / Solution**:
```cpp
// RAII パターンを使用 / Use RAII pattern
class PooledObjectGuard {
public:
    PooledObjectGuard(ObjectPools& pools, Object* obj)
        : pools_(pools), obj_(obj) {}
    
    ~PooledObjectGuard() {
        if(obj_) pools_.release(obj_);
    }
    
private:
    ObjectPools& pools_;
    Object* obj_;
};

// 使用 / Usage
{
    PooledObjectGuard guard(pools, pools.acquire<Object>());
    // オブジェクトを使用 / Use object
} // 自動的に返却 / Automatically returned
```

### 問題: パフォーマンスが遅い / Problem: Slow Performance

**解決策 / Solutions**:
1. オブジェクトプールを使用 / Use object pools
2. 不要なコピーを避ける / Avoid unnecessary copies
3. インライン化を活用 / Utilize inlining
4. プロファイリングでボトルネックを特定 / Identify bottlenecks with profiling

## ライブラリの拡張 / Extending the Library

### 新しい交叉オペレータの追加 / Adding New Crossover Operators

```cpp
namespace mpi::eax {

class CustomEAX {
public:
    CustomEAX(const DistanceMatrix& dist_matrix)
        : dist_matrix_(dist_matrix) {}
    
    Tour crossover(const Tour& parent1, const Tour& parent2) {
        // カスタム交叉ロジック / Custom crossover logic
        ABCycleBuilder builder;
        auto ab_cycles = builder.build(parent1, parent2, dist_matrix_);
        
        // カスタムE-set選択 / Custom E-set selection
        CustomESetSelector selector;
        auto e_set = selector.select(ab_cycles);
        
        // 子を生成 / Generate offspring
        return e_set.apply(parent1);
    }
    
private:
    const DistanceMatrix& dist_matrix_;
};

} // namespace mpi::eax
```

## 関連項目 / See Also

- **[EAX](EAX)** - EAXアルゴリズムの概要 / Overview of EAX algorithms
- **[Fast EAX](Fast-EAX)** - eaxlibを使用した高速実装 / Fast implementation using eaxlib
- **[EAX STSP](EAX-STSP)** - eaxlibを使用したSTSP実装 / STSP implementation using eaxlib
- **[API Reference](API-Reference)** - 完全なAPIドキュメント / Complete API documentation

## 参考文献 / References

- Nagata, Y., & Kobayashi, S. (1997). "Edge Assembly Crossover: A High-Power Genetic Algorithm for the Traveling Salesman Problem."
- Nagata, Y. (2007). "Edge Assembly Crossover for the Capacitated Vehicle Routing Problem."
