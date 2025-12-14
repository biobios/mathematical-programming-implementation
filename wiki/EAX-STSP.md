# EAX STSP (Symmetric TSP)

## 概要 / Overview

EAX STSPは、対称巡回セールスマン問題（Symmetric TSP）に特化したEAXアルゴリズムの実装です。対称TSPでは、都市i→jの距離と都市j→iの距離が等しいという特性を活用して最適化されています。

EAX STSP is an implementation of the EAX algorithm specialized for Symmetric Traveling Salesman Problems (STSP). It is optimized by leveraging the property that the distance from city i to j equals the distance from j to i in symmetric TSP.

## ディレクトリ / Directory

```
src/eax_stsp/
```

## ビルド / Build

```bash
# 通常ビルド / Normal build
make bin/eax_stsp

# デバッグビルド / Debug build
make bin/debug/eax_stsp

# プロファイリングビルド / Profiling build
make bin/prof/eax_stsp
```

## 使い方 / Usage

### 基本的な実行 / Basic Execution

```bash
ARGS="--file data/eax_stsp/att532.tsp" make run/eax_stsp
```

### コマンドライン引数 / Command-Line Arguments

```bash
--file <filename>           # TSPファイルのパス / Path to TSP file (required)
--ps <size>                 # 集団サイズ / Population size
                           # デフォルト: 0 (自動設定) / Default: 0 (auto)
--generations <number>      # 世代数 / Number of generations
                           # デフォルト: 300 / Default: 300
--trials <number>           # 試行回数 / Number of trials
                           # デフォルト: 1 / Default: 1
--seed <value>              # 乱数シード / Random seed
                           # デフォルト: システム依存 / Default: system-dependent
--help                      # ヘルプメッセージを表示 / Show help message
```

### 実行例 / Examples

```bash
# ヘルプを表示 / Show help
ARGS="--help" make run/eax_stsp

# 基本的な実行 / Basic run
ARGS="--file data/eax_stsp/att532.tsp" make run/eax_stsp

# 集団サイズと世代数を指定 / Specify population size and generations
ARGS="--file data/eax_stsp/att532.tsp --ps 100 --generations 500" make run/eax_stsp

# 複数試行を実行 / Multiple trials
ARGS="--file data/eax_stsp/att532.tsp --ps 200 --trials 10" make run/eax_stsp

# 特定のシード値で実行 / Run with specific seed
ARGS="--file data/eax_stsp/att532.tsp --seed 42" make run/eax_stsp

# プロファイリング実行 / Profiling run
ARGS="--file data/eax_stsp/att532.tsp --ps 100" make prof_run/eax_stsp
```

## 特徴 / Features

### 対称性の活用 / Exploiting Symmetry

対称TSPでは、距離行列が対称であるため:
- メモリ使用量を削減可能 / Memory usage can be reduced
- 計算の最適化が可能 / Computational optimization is possible
- ツアーの表現を簡略化 / Tour representation can be simplified

In symmetric TSP, since the distance matrix is symmetric:
- Memory usage can be reduced
- Computational optimization is possible
- Tour representation can be simplified

### エリート保存戦略 / Elitist Preservation

最良個体を常に保持し、解の質を保証します。

Always preserve the best individual to guarantee solution quality.

### 2-opt局所探索 / 2-opt Local Search

生成された子個体に2-opt局所探索を適用して、ツアーを改善します。

Apply 2-opt local search to generated offspring to improve tours.

## アルゴリズムの詳細 / Algorithm Details

### 初期化 / Initialization

1. TSPファイルからインスタンスを読み込み / Load instance from TSP file
2. 距離行列を計算 / Calculate distance matrix
3. 初期集団をランダムに生成 / Generate initial population randomly
4. 各個体に2-optを適用 / Apply 2-opt to each individual

### メインループ / Main Loop

各世代で:

For each generation:

1. **親選択 / Parent Selection**:
   - トーナメント選択または適応度比例選択 / Tournament or fitness-proportional selection
   
2. **交叉 / Crossover**:
   - EAX交叉オペレータを適用 / Apply EAX crossover operator
   - AB-cycleを構築 / Construct AB-cycles
   - 子個体を生成 / Generate offspring
   
3. **局所探索 / Local Search**:
   - 2-optで子個体を改善 / Improve offspring with 2-opt
   
4. **集団更新 / Population Update**:
   - 子個体が親より良ければ集団に追加 / Add offspring to population if better than parents
   - 集団サイズを維持 / Maintain population size

### 終了条件 / Termination

以下のいずれかで終了:
- 指定世代数に到達 / Reach specified number of generations
- 収束（改善がない）/ Convergence (no improvement)

## パフォーマンス / Performance

### 推奨パラメータ / Recommended Parameters

問題サイズに応じた推奨設定:

Recommended settings based on problem size:

| 都市数 / Cities | 集団サイズ / Pop Size | 世代数 / Generations |
|----------------|----------------------|---------------------|
| < 100 | 50-100 | 300-500 |
| 100-300 | 100-200 | 500-1000 |
| 300-500 | 200-300 | 1000-2000 |
| > 500 | 300-500 | 2000+ |

### 計算時間 / Computation Time

att532問題（532都市）での実行時間の目安:

Approximate execution time for att532 problem (532 cities):

- 集団サイズ100、300世代: 約1-3分 / ~1-3 minutes
- 集団サイズ200、1000世代: 約10-20分 / ~10-20 minutes

（ハードウェアにより異なります / Varies by hardware）

## データファイル / Data Files

プロジェクトには、以下のサンプルデータが含まれています:

The project includes the following sample data:

```
data/eax_stsp/
├── att532.tsp      # 532都市問題 / 532-city problem
└── ...
```

## トラブルシューティング / Troubleshooting

### メモリ不足 / Out of Memory

大きな問題で集団サイズが大きすぎる場合:
- 集団サイズを減らす / Reduce population size
- 64ビットビルドを使用 / Use 64-bit build

### 収束が遅い / Slow Convergence

- 集団サイズを増やす / Increase population size
- 世代数を増やす / Increase number of generations
- 異なるシード値を試す / Try different seed values

### ファイルが見つからない / File Not Found

ファイルパスを確認:
```bash
ls -la data/eax_stsp/
```

## 出力形式 / Output Format

プログラムは以下の情報を出力します:

The program outputs the following information:

```
Loading TSP file: data/eax_stsp/att532.tsp
Number of cities: 532
Population size: 100
Generations: 300

Generation 0: Best = 27686
Generation 50: Best = 27432
Generation 100: Best = 27234
Generation 150: Best = 27123
Generation 200: Best = 27089
Generation 250: Best = 27086
Generation 300: Best = 27086

Best tour length: 27086
Time: 125.43 seconds
```

## 関連項目 / See Also

- **[EAX](EAX)** - EAXアルゴリズムの概要 / Overview of EAX algorithms
- **[Fast EAX](Fast-EAX)** - より高速なバージョン / Faster version
- **[eaxlib](eaxlib)** - EAXライブラリのAPIリファレンス / EAX library API reference
- **[Getting Started](Getting-Started)** - はじめに / Getting started guide
