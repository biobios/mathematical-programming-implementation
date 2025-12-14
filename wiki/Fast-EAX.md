# Fast EAX

## 概要 / Overview

Fast EAXは、EAXアルゴリズムの高速化バージョンです。様々な最適化技術を用いて、大規模なTSP問題を高速に解くことができます。

Fast EAX is an optimized, high-speed version of the EAX algorithm. It uses various optimization techniques to solve large-scale TSP problems quickly.

## ディレクトリ / Directory

```
src/fast_eax/
```

## ビルド / Build

```bash
# 通常ビルド / Normal build
make bin/fast_eax

# デバッグビルド / Debug build
make bin/debug/fast_eax

# プロファイリングビルド / Profiling build
make bin/prof/fast_eax
```

## 使い方 / Usage

### 基本的な実行 / Basic Execution

```bash
ARGS="--file data/fast_eax/att532.tsp" make run/fast_eax
```

### コマンドライン引数 / Command-Line Arguments

```bash
--file <filename>              # TSPファイルのパス / Path to TSP file (required)
--ps, --population-size <size> # 集団サイズ / Population size
                               # デフォルト: 0 (自動設定) / Default: 0 (auto)
--trials <number>              # 試行回数 / Number of trials
                               # デフォルト: 1 / Default: 1
--seed <value>                 # 乱数シード / Random seed
--neighbor-2opt                # 近傍2-optを使用 (デフォルト) / Use neighbor 2-opt (default)
--global-2opt                  # グローバル2-optを使用 / Use global 2-opt
--local-eax                    # 局所EAXを使用 (デフォルト) / Use local EAX (default)
--global-eax                   # グローバルEAXを使用 / Use global EAX
--greedy-selection             # 貪欲選択を使用 / Use greedy selection
--ent-selection                # エントロピー選択を使用 (デフォルト) / Use entropy selection (default)
--help                         # ヘルプメッセージを表示 / Show help message
```

### 実行例 / Examples

```bash
# ヘルプを表示 / Show help
ARGS="--help" make run/fast_eax

# 基本的な実行 / Basic run
ARGS="--file data/fast_eax/att532.tsp" make run/fast_eax

# 集団サイズを指定 / Specify population size
ARGS="--file data/fast_eax/att532.tsp --ps 200" make run/fast_eax

# グローバル2-optを使用 / Use global 2-opt
ARGS="--file data/fast_eax/att532.tsp --global-2opt" make run/fast_eax

# 貪欲選択を使用 / Use greedy selection
ARGS="--file data/fast_eax/att532.tsp --greedy-selection" make run/fast_eax

# 複数試行を実行 / Multiple trials
ARGS="--file data/fast_eax/att532.tsp --ps 150 --trials 5" make run/fast_eax

# すべてのオプションを組み合わせ / Combine all options
ARGS="--file data/fast_eax/att532.tsp --ps 200 --global-eax --global-2opt --greedy-selection --seed 42" make run/fast_eax
```

## 最適化技術 / Optimization Techniques

### 1. 高速AB-cycle構築 / Fast AB-cycle Construction

従来のEAXよりも高速にAB-cycleを構築します:
- 効率的なデータ構造の使用 / Use efficient data structures
- 不要な計算の削減 / Reduce unnecessary computations

Fast AB-cycle construction compared to traditional EAX:
- Using efficient data structures
- Reducing unnecessary computations

### 2. 局所vs.グローバルEAX / Local vs. Global EAX

**局所EAX (デフォルト) / Local EAX (default)**:
- AB-cycleの構築範囲を制限 / Limit AB-cycle construction range
- 高速だが解の質は若干低下 / Faster but slightly lower solution quality

**グローバルEAX / Global EAX**:
- 全範囲でAB-cycleを構築 / Construct AB-cycles over full range
- 遅いが解の質が高い / Slower but higher solution quality

### 3. 2-opt最適化 / 2-opt Optimization

**近傍2-opt (デフォルト) / Neighbor 2-opt (default)**:
- 近接する都市間のみ交換を試行 / Try swaps only between nearby cities
- 大幅に高速 / Significantly faster
- 大規模問題に適している / Suitable for large problems

**グローバル2-opt / Global 2-opt**:
- すべての都市間で交換を試行 / Try swaps between all city pairs
- 完全だが遅い / Complete but slower
- 小規模問題に適している / Suitable for small problems

### 4. 選択戦略 / Selection Strategy

**エントロピー選択 (デフォルト) / Entropy Selection (default)**:
- 集団の多様性を維持 / Maintain population diversity
- 早期収束を防ぐ / Prevent premature convergence
- 長期実行に適している / Suitable for long runs

**貪欲選択 / Greedy Selection**:
- 最良個体を優先的に選択 / Preferentially select best individuals
- 高速に収束 / Converge quickly
- 短期実行に適している / Suitable for short runs

## パフォーマンス比較 / Performance Comparison

### 速度比較 / Speed Comparison

att532問題での実行時間（集団サイズ200）:

Execution time for att532 problem (population size 200):

| 設定 / Configuration | 時間 / Time | 解の質 / Quality |
|---------------------|------------|----------------|
| local-eax + neighbor-2opt (デフォルト) | 30秒 / 30s | 良好 / Good |
| local-eax + global-2opt | 2分 / 2min | 非常に良好 / Very Good |
| global-eax + neighbor-2opt | 1分 / 1min | 非常に良好 / Very Good |
| global-eax + global-2opt | 5分 / 5min | 最良 / Best |

### 推奨設定 / Recommended Settings

**高速実行 / Fast Execution**:
```bash
--local-eax --neighbor-2opt --greedy-selection
```

**バランス型 / Balanced**:
```bash
--local-eax --neighbor-2opt --ent-selection  # (デフォルト / default)
```

**高品質解 / High-Quality Solution**:
```bash
--global-eax --global-2opt --ent-selection
```

## アーキテクチャ / Architecture

### 主要なコンポーネント / Main Components

```
fast_eax/
├── main.cpp                    # エントリーポイント / Entry point
├── individual.hpp              # 個体の表現 / Individual representation
├── individual.cpp              # 個体の実装 / Individual implementation
├── generational_model.hpp      # 世代モデル / Generational model
└── environment.hpp             # 環境設定 / Environment settings
```

### データ構造 / Data Structures

**Individual (個体) / Individual**:
- ツアーの表現（隣接リスト形式） / Tour representation (adjacency list format)
- 適応度値 / Fitness value
- 遺伝的情報 / Genetic information

**Environment (環境) / Environment**:
- 距離行列 / Distance matrix
- アルゴリズムパラメータ / Algorithm parameters
- オブジェクトプール / Object pools

## メモリ管理 / Memory Management

Fast EAXは、以下のメモリ最適化を実装:

Fast EAX implements the following memory optimizations:

- **オブジェクトプール / Object Pools**: 頻繁に生成/破棄されるオブジェクトを再利用 / Reuse frequently created/destroyed objects
- **効率的なデータ構造 / Efficient Data Structures**: メモリアクセスパターンの最適化 / Optimize memory access patterns
- **キャッシュフレンドリー / Cache-Friendly**: CPUキャッシュを効率的に使用 / Efficiently use CPU cache

## ベンチマーク / Benchmarks

### 標準的な問題での結果 / Results on Standard Problems

| 問題 / Problem | 都市数 / Cities | 最適解 / Optimal | Fast EAX (平均) | 誤差 / Error |
|---------------|----------------|-----------------|----------------|-------------|
| att532 | 532 | 27686 | 27920 | 0.85% |
| pr1002 | 1002 | 259045 | 262300 | 1.26% |
| d1291 | 1291 | 50801 | 51450 | 1.28% |

（集団サイズ200、デフォルト設定、10試行の平均）

(Population size 200, default settings, average of 10 trials)

## カスタマイズ / Customization

### パラメータのチューニング / Parameter Tuning

問題の特性に応じてパラメータを調整:

Adjust parameters based on problem characteristics:

1. **密集した都市 / Clustered Cities**: 
   - `--neighbor-2opt`を使用 / Use `--neighbor-2opt`
   
2. **均等分布 / Uniform Distribution**:
   - `--global-2opt`が効果的 / `--global-2opt` is effective
   
3. **時間制限あり / Time Limited**:
   - `--greedy-selection`で高速収束 / Fast convergence with `--greedy-selection`
   
4. **高品質解が必要 / High Quality Needed**:
   - `--ent-selection`で多様性を維持 / Maintain diversity with `--ent-selection`

## トラブルシューティング / Troubleshooting

### 収束しない / Not Converging

- 集団サイズを増やす: `--ps 300` / Increase population size
- エントロピー選択を使用: `--ent-selection` / Use entropy selection

### メモリ使用量が多い / High Memory Usage

- 集団サイズを減らす / Reduce population size
- 近傍2-optを使用（メモリ効率的） / Use neighbor 2-opt (memory efficient)

### 遅すぎる / Too Slow

- 局所EAXを使用: `--local-eax` / Use local EAX
- 近傍2-optを使用: `--neighbor-2opt` / Use neighbor 2-opt
- 貪欲選択を使用: `--greedy-selection` / Use greedy selection

## 関連項目 / See Also

- **[EAX](EAX)** - EAXアルゴリズムの概要 / Overview of EAX algorithms
- **[EAX STSP](EAX-STSP)** - 対称TSP版 / Symmetric TSP version
- **[Normal EAX](Normal-EAX)** - 標準実装 / Standard implementation
- **[eaxlib](eaxlib)** - EAXライブラリのAPIリファレンス / EAX library API reference
