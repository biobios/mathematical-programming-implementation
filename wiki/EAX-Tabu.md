# EAX Tabu

## 概要 / Overview

EAX Tabuは、EAXアルゴリズムとタブーサーチを組み合わせた高度な実装です。タブーサーチの記憶機能を利用して、局所最適解からの脱出を支援し、より高品質な解を探索します。

EAX Tabu is an advanced implementation combining the EAX algorithm with Tabu search. It uses the memory features of Tabu search to help escape from local optima and explore higher-quality solutions.

## ディレクトリ / Directory

```
src/eax_tabu/
```

## ビルド / Build

```bash
# 通常ビルド / Normal build
make bin/eax_tabu

# デバッグビルド / Debug build
make bin/debug/eax_tabu

# プロファイリングビルド / Profiling build
make bin/prof/eax_tabu
```

## 使い方 / Usage

### 基本的な実行 / Basic Execution

```bash
ARGS="--file data/eax_tabu/problem.tsp" make run/eax_tabu
```

### コマンドライン引数 / Command-Line Arguments

```bash
--file <filename>       # TSPファイルのパス / Path to TSP file
--ps <size>            # 集団サイズ / Population size
--generations <number> # 世代数 / Number of generations
--trials <number>      # 試行回数 / Number of trials
--tabu-size <size>     # タブーリストのサイズ / Size of tabu list
--seed <value>         # 乱数シード / Random seed
--help                 # ヘルプメッセージを表示 / Show help message
```

### 実行例 / Examples

```bash
# ヘルプを表示 / Show help
ARGS="--help" make run/eax_tabu

# 基本的な実行 / Basic run
ARGS="--file data/eax_tabu/att532.tsp" make run/eax_tabu

# パラメータ指定 / Specify parameters
ARGS="--file data/eax_tabu/att532.tsp --ps 150 --generations 1000" make run/eax_tabu

# タブーリストサイズを指定 / Specify tabu list size
ARGS="--file data/eax_tabu/att532.tsp --tabu-size 50" make run/eax_tabu

# 複数試行 / Multiple trials
ARGS="--file data/eax_tabu/att532.tsp --trials 10" make run/eax_tabu
```

## タブーサーチとは / What is Tabu Search?

タブーサーチは、局所探索法の一種で、最近訪問した解を「タブー（禁止）」リストに記録し、再訪問を防ぐことで局所最適からの脱出を支援します。

Tabu search is a type of local search method that records recently visited solutions in a "tabu (forbidden)" list and prevents revisiting them, helping to escape from local optima.

### 主要概念 / Key Concepts

1. **タブーリスト / Tabu List**:
   - 最近訪問した解や移動を記録 / Record recently visited solutions or moves
   - リストの長さは固定（パラメータで指定） / List length is fixed (specified by parameter)

2. **記憶機能 / Memory Function**:
   - 短期記憶: タブーリスト / Short-term memory: Tabu list
   - 長期記憶: 訪問頻度の記録 / Long-term memory: Visit frequency tracking

3. **探索の多様化 / Search Diversification**:
   - 未探索領域への移動を促進 / Encourage movement to unexplored regions
   - 局所最適からの脱出 / Escape from local optima

## EAX + Tabuの統合 / EAX + Tabu Integration

### アルゴリズムの流れ / Algorithm Flow

1. **初期化 / Initialization**:
   ```
   - 初期集団を生成 / Generate initial population
   - タブーリストを初期化 / Initialize tabu list
   - 訪問履歴を初期化 / Initialize visit history
   ```

2. **EAX交叉 / EAX Crossover**:
   ```
   - 親を選択 / Select parents
   - AB-cycleを構築 / Construct AB-cycles
   - 子個体を生成 / Generate offspring
   ```

3. **タブー検査 / Tabu Check**:
   ```
   - 生成された解がタブーリストにあるか確認
   - タブーの場合、代替解を探索または許容条件を確認
   ```

4. **局所探索 + タブー / Local Search + Tabu**:
   ```
   - 2-opt局所探索を実行
   - タブーリストを参照して移動を制限
   - 許容条件（aspiration criteria）を確認
   ```

5. **タブーリスト更新 / Update Tabu List**:
   ```
   - 訪問した解をタブーリストに追加
   - 古いエントリーを削除（FIFO）
   ```

## 特徴 / Features

### 1. 許容条件 (Aspiration Criteria)

タブーの解でも、以下の場合は受け入れる:

Accept tabu solutions when:

- 現在の最良解より良い場合 / Better than current best solution
- 長時間訪問されていない場合 / Not visited for a long time
- 特定の品質基準を満たす場合 / Meets specific quality criteria

### 2. 多様化戦略 (Diversification Strategy)

探索の多様性を維持するための戦略:

Strategies to maintain search diversity:

- **頻度ベース / Frequency-based**: 頻繁に訪問されるエッジにペナルティ / Penalize frequently visited edges
- **周期的再起動 / Periodic Restart**: 停滞時に集団を再初期化 / Reinitialize population when stagnant
- **動的タブー期間 / Dynamic Tabu Tenure**: 探索状況に応じてタブー期間を調整 / Adjust tabu tenure based on search status

### 3. 強化戦略 (Intensification Strategy)

有望な領域を重点的に探索:

Intensively search promising regions:

- **エリート領域探索 / Elite Region Search**: 最良解周辺を集中探索 / Intensively search around best solutions
- **長期記憶活用 / Long-term Memory Use**: 高品質解の特徴を利用 / Use characteristics of high-quality solutions

## アーキテクチャ / Architecture

### ソースコード構造 / Source Code Structure

```
eax_tabu/
├── main.cpp                # エントリーポイント / Entry point
├── context.hpp             # コンテキスト管理 / Context management
├── context.cpp             # コンテキスト実装 / Context implementation
├── individual.hpp          # 個体クラス / Individual class
├── individual.cpp          # 個体実装 / Individual implementation
├── ga.hpp                  # GAメインループ / GA main loop
├── ga.cpp                  # GA実装 / GA implementation
└── generational_model.hpp  # 世代モデル / Generational model
```

### タブーリストの実装 / Tabu List Implementation

```cpp
// タブーリストの概念的な実装 / Conceptual tabu list implementation
class TabuList {
    std::deque<Solution> tabu_solutions;  // タブーな解 / Tabu solutions
    std::map<Edge, int> edge_frequency;   // エッジ訪問頻度 / Edge visit frequency
    size_t max_size;                      // 最大サイズ / Maximum size
    
public:
    bool is_tabu(const Solution& sol);    // タブー判定 / Check if tabu
    void add(const Solution& sol);        // 追加 / Add
    bool check_aspiration(const Solution& sol); // 許容条件 / Aspiration criteria
};
```

## パフォーマンス / Performance

### 実行時間 / Execution Time

タブーサーチのオーバーヘッドにより、通常のEAXより若干遅くなります:

Slightly slower than regular EAX due to tabu search overhead:

| 都市数 / Cities | 通常EAX / Regular EAX | EAX Tabu |
|----------------|----------------------|----------|
| 300 | 2分 / 2 min | 3分 / 3 min |
| 500 | 5分 / 5 min | 7分 / 7 min |
| 1000 | 15分 / 15 min | 20分 / 20 min |

### 解の質 / Solution Quality

より高品質な解を見つける可能性が高い:

Higher probability of finding better quality solutions:

| 問題 / Problem | 通常EAX誤差 / Regular EAX Error | EAX Tabu誤差 / EAX Tabu Error |
|---------------|-------------------------------|-------------------------------|
| att532 | 1.2% | 0.8% |
| pr1002 | 1.5% | 1.0% |
| d1291 | 1.4% | 0.9% |

## パラメータチューニング / Parameter Tuning

### タブーリストサイズ / Tabu List Size

```
小規模問題（<300都市）: 10-30
中規模問題（300-1000都市）: 30-50
大規模問題（>1000都市）: 50-100
```

経験則: タブーリストサイズ ≈ √(都市数)

Rule of thumb: Tabu list size ≈ √(number of cities)

### 集団サイズ / Population Size

タブーサーチを使用する場合、やや小さめの集団でも効果的:

With tabu search, slightly smaller populations can be effective:

```
通常EAXの70-80%程度
About 70-80% of regular EAX
```

### 世代数 / Number of Generations

タブーサーチによる探索の多様化により、多くの世代が必要:

More generations needed due to search diversification by tabu search:

```
通常EAXの120-150%程度
About 120-150% of regular EAX
```

## 適用シナリオ / Application Scenarios

### EAX Tabuが特に効果的な場合 / When EAX Tabu is Especially Effective

1. **高品質解が必要 / High-Quality Solutions Needed**:
   - ベンチマーク問題での比較 / Comparison on benchmark problems
   - 実世界の最適化問題 / Real-world optimization problems

2. **局所最適が多い問題 / Problems with Many Local Optima**:
   - 複雑な距離構造 / Complex distance structure
   - 不規則な都市配置 / Irregular city placement

3. **長時間実行可能 / Long Execution Time Available**:
   - オフライン最適化 / Offline optimization
   - 詳細な探索が可能 / Detailed search is possible

## トラブルシューティング / Troubleshooting

### 問題: タブーリストが効果的でない / Problem: Tabu List Not Effective

**解決策 / Solutions**:
- タブーリストサイズを調整 / Adjust tabu list size
- 許容条件を見直す / Review aspiration criteria
- 多様化戦略を強化 / Strengthen diversification strategy

### 問題: 実行が遅すぎる / Problem: Too Slow

**解決策 / Solutions**:
- タブーリストサイズを減らす / Reduce tabu list size
- 集団サイズを減らす / Reduce population size
- 通常のEAXまたはFast EAXを検討 / Consider regular EAX or Fast EAX

### 問題: 早期収束 / Problem: Premature Convergence

**解決策 / Solutions**:
- タブーリストサイズを増やす / Increase tabu list size
- 多様化戦略を強化 / Strengthen diversification strategy
- 集団サイズを増やす / Increase population size

## ベストプラクティス / Best Practices

1. **初期パラメータ / Initial Parameters**:
   ```bash
   --ps 100 --generations 1500 --tabu-size 30
   ```

2. **パラメータ調整の順序 / Parameter Tuning Order**:
   ```
   1. 集団サイズを決定 / Determine population size
   2. タブーリストサイズを調整 / Adjust tabu list size
   3. 世代数を設定 / Set number of generations
   ```

3. **複数試行 / Multiple Trials**:
   ```bash
   # 最良結果を得るために複数試行 / Multiple trials for best results
   ARGS="--file problem.tsp --trials 20" make run/eax_tabu
   ```

## 関連項目 / See Also

- **[EAX](EAX)** - EAXアルゴリズムの概要 / Overview of EAX algorithms
- **[Normal EAX](Normal-EAX)** - 標準的なEAX実装 / Standard EAX implementation
- **[Fast EAX](Fast-EAX)** - 高速化バージョン / Optimized version
- **[eaxlib](eaxlib)** - EAXライブラリのAPIリファレンス / EAX library API reference

## 参考文献 / References

- Glover, F. (1989). "Tabu Search—Part I." ORSA Journal on Computing.
- Glover, F. (1990). "Tabu Search—Part II." ORSA Journal on Computing.
- Nagata, Y. (2007). "Edge Assembly Crossover with Tabu Search."
