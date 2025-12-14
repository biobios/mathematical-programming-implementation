# EAX (Edge Assembly Crossover) アルゴリズム

## 概要 / Overview

EAX（Edge Assembly Crossover）は、巡回セールスマン問題（TSP）を解くための効果的な遺伝的アルゴリズムの交叉オペレータです。このプロジェクトでは、EAXアルゴリズムの複数のバリエーションを提供しています。

EAX (Edge Assembly Crossover) is an effective crossover operator for genetic algorithms solving the Traveling Salesman Problem (TSP). This project provides multiple variants of the EAX algorithm.

## TSP問題 / TSP Problem

巡回セールスマン問題は、すべての都市を一度だけ訪問し、出発点に戻る最短経路を見つける問題です。

The Traveling Salesman Problem is to find the shortest route that visits all cities exactly once and returns to the starting point.

## EAXバリエーション / EAX Variants

このプロジェクトには、以下のEAXバリエーションが含まれています:

This project includes the following EAX variants:

- **[EAX STSP](EAX-STSP)** - 対称TSP用のEAX / EAX for Symmetric TSP
- **[Fast EAX](Fast-EAX)** - 最適化された高速EAX / Optimized fast EAX
- **[Normal EAX](Normal-EAX)** - 標準的なEAX実装 / Standard EAX implementation
- **[EAX Tabu](EAX-Tabu)** - Tabuサーチと組み合わせたEAX / EAX combined with Tabu search
- **EAX (ATSP)** - 非対称TSP用のEAX / EAX for Asymmetric TSP

## 基本的なアルゴリズム / Basic Algorithm

EAXアルゴリズムは以下のステップで動作します:

The EAX algorithm works with the following steps:

1. **親の選択 / Parent Selection**: 集団から2つの親個体を選択 / Select two parent individuals from the population

2. **E-set構築 / E-set Construction**: 2つの親のツアーからエッジを組み合わせてAB-cycleを構築 / Construct AB-cycles by combining edges from the two parent tours

3. **子の生成 / Offspring Generation**: AB-cycleを選択して子個体を生成 / Generate offspring by selecting AB-cycles

4. **局所探索 / Local Search**: 2-optなどの局所探索で子個体を改善 / Improve offspring with local search such as 2-opt

5. **集団更新 / Population Update**: 良い子個体を集団に追加 / Add good offspring to the population

## 共通の特徴 / Common Features

### 遺伝的アルゴリズムのフレームワーク / Genetic Algorithm Framework

すべてのEAX実装は以下の共通フレームワークを使用:

All EAX implementations use the following common framework:

- **世代モデル / Generational Model**: エリート保存戦略 / Elitist preservation strategy
- **適応度評価 / Fitness Evaluation**: ツアー長の逆数 / Inverse of tour length
- **集団初期化 / Population Initialization**: ランダムまたは貪欲法による初期化 / Random or greedy initialization
- **終了条件 / Termination Criteria**: 世代数または収束 / Number of generations or convergence

### 2-opt局所探索 / 2-opt Local Search

すべてのバリエーションで2-opt局所探索を使用してツアーを改善:

All variants use 2-opt local search to improve tours:

- **近傍2-opt / Neighbor 2-opt**: 近い都市間のみを考慮（高速） / Consider only nearby cities (faster)
- **グローバル2-opt / Global 2-opt**: すべての都市間を考慮（完全だが遅い） / Consider all city pairs (complete but slower)

## データフォーマット / Data Format

### TSPLIBフォーマット / TSPLIB Format

EAXアルゴリズムは、TSPLIB形式のファイルを読み込みます:

The EAX algorithms load TSPLIB format files:

```
NAME: example532
TYPE: TSP
COMMENT: Example 532-city problem
DIMENSION: 532
EDGE_WEIGHT_TYPE: EUC_2D
NODE_COORD_SECTION
1 0.0 0.0
2 1.0 2.0
...
EOF
```

### サポートされているエッジ重みタイプ / Supported Edge Weight Types

- `EUC_2D`: 2次元ユークリッド距離 / 2D Euclidean distance
- `GEO`: 地理的距離 / Geographical distance
- `ATT`: 疑似ユークリッド距離 / Pseudo-Euclidean distance

## 一般的なコマンドライン引数 / Common Command-Line Arguments

```bash
--file <filename>           # TSPファイルのパス / Path to TSP file
--ps <size>                 # 集団サイズ / Population size
--generations <number>      # 世代数 / Number of generations
--trials <number>           # 試行回数 / Number of trials
--seed <value>              # 乱数シード / Random seed
--help                      # ヘルプメッセージを表示 / Show help message
```

## パフォーマンスのヒント / Performance Tips

### 集団サイズの選択 / Choosing Population Size

- 小さい問題（<100都市）: 50-100 / Small problems (<100 cities): 50-100
- 中規模問題（100-500都市）: 100-300 / Medium problems (100-500 cities): 100-300
- 大きい問題（>500都市）: 300-1000 / Large problems (>500 cities): 300-1000

### 世代数の選択 / Choosing Number of Generations

- 探索的実行: 100-500世代 / Exploratory runs: 100-500 generations
- 本格的な最適化: 1000+世代 / Serious optimization: 1000+ generations

### コンパイル最適化 / Compilation Optimization

最高のパフォーマンスを得るには、最適化フラグを使用:

For best performance, use optimization flags:

```bash
make bin/fast_eax           # -O3 -flto で最適化済み / Optimized with -O3 -flto
```

## アルゴリズムの比較 / Algorithm Comparison

| バリエーション / Variant | 速度 / Speed | 解の質 / Quality | 用途 / Use Case |
|-------------------------|--------------|------------------|-----------------|
| Fast EAX | 非常に高速 / Very Fast | 良好 / Good | 大規模問題、高速実行 / Large problems, quick runs |
| Normal EAX | 中速 / Medium | 非常に良好 / Very Good | 一般的な使用 / General use |
| EAX STSP | 高速 / Fast | 良好 / Good | 対称TSP / Symmetric TSP |
| EAX Tabu | 中速 / Medium | 非常に良好 / Very Good | 高品質解が必要 / High-quality solutions needed |

## 例 / Examples

### 基本的な実行例 / Basic Execution Example

```bash
# att532問題を解く（集団サイズ100、300世代）
# Solve att532 problem (population size 100, 300 generations)
ARGS="--file data/eax_stsp/att532.tsp --ps 100 --generations 300" make run/eax_stsp
```

### 複数試行での実行 / Multiple Trials

```bash
# 10回の試行を実行
# Run 10 trials
ARGS="--file data/fast_eax/att532.tsp --ps 200 --trials 10" make run/fast_eax
```

### カスタムシードでの実行 / Custom Seed

```bash
# 特定のシード値で実行
# Run with specific seed
ARGS="--file data/fast_eax/att532.tsp --seed 12345" make run/fast_eax
```

## 結果の解釈 / Interpreting Results

プログラムは以下を出力します:

The program outputs:

- **最良ツアー長 / Best Tour Length**: 見つかった最短ツアーの長さ / Length of the shortest tour found
- **実行時間 / Execution Time**: アルゴリズムの実行時間 / Time taken by the algorithm
- **世代情報 / Generation Info**: 各世代での進捗 / Progress in each generation

## 参考文献 / References

- Nagata, Y., & Kobayashi, S. (1997). "Edge Assembly Crossover: A High-Power Genetic Algorithm for the Traveling Salesman Problem."
- Nagata, Y., & Kobayashi, S. (2013). "A Powerful Genetic Algorithm Using Edge Assembly Crossover for the Traveling Salesman Problem."

## 次のステップ / Next Steps

各バリエーションの詳細については、個別のページを参照してください:

For details on each variant, see the individual pages:

- **[EAX STSP](EAX-STSP)** - 対称TSP用の実装詳細 / Implementation details for Symmetric TSP
- **[Fast EAX](Fast-EAX)** - 高速バージョンの詳細 / Details of the fast version
- **[Normal EAX](Normal-EAX)** - 標準実装の詳細 / Details of the standard implementation
- **[EAX Tabu](EAX-Tabu)** - Tabuサーチとの組み合わせ / Combination with Tabu search
- **[eaxlib](eaxlib)** - EAXライブラリのAPIリファレンス / EAX library API reference
