# Normal EAX

## 概要 / Overview

Normal EAXは、EAXアルゴリズムの標準的な実装です。速度と解の質のバランスが取れており、一般的なTSP問題の解決に適しています。

Normal EAX is a standard implementation of the EAX algorithm. It provides a good balance between speed and solution quality, making it suitable for solving general TSP problems.

## ディレクトリ / Directory

```
src/normal_eax/
```

## ビルド / Build

```bash
# 通常ビルド / Normal build
make bin/normal_eax

# デバッグビルド / Debug build
make bin/debug/normal_eax

# プロファイリングビルド / Profiling build
make bin/prof/normal_eax
```

## 使い方 / Usage

### 基本的な実行 / Basic Execution

```bash
ARGS="--file data/normal_eax/problem.tsp" make run/normal_eax
```

### コマンドライン引数 / Command-Line Arguments

Normal EAXは、標準的なEAXパラメータをサポートしています:

Normal EAX supports standard EAX parameters:

```bash
--file <filename>       # TSPファイルのパス / Path to TSP file
--ps <size>            # 集団サイズ / Population size
--generations <number> # 世代数 / Number of generations
--trials <number>      # 試行回数 / Number of trials
--seed <value>         # 乱数シード / Random seed
--help                 # ヘルプメッセージを表示 / Show help message
```

### 実行例 / Examples

```bash
# ヘルプを表示 / Show help
ARGS="--help" make run/normal_eax

# 基本的な実行 / Basic run
ARGS="--file data/normal_eax/att532.tsp" make run/normal_eax

# パラメータ指定 / Specify parameters
ARGS="--file data/normal_eax/att532.tsp --ps 150 --generations 500" make run/normal_eax

# 複数試行 / Multiple trials
ARGS="--file data/normal_eax/att532.tsp --trials 10" make run/normal_eax
```

## 特徴 / Features

### 標準的なEAX実装 / Standard EAX Implementation

- **AB-cycle構築 / AB-cycle Construction**: 標準的な手法でAB-cycleを生成 / Generate AB-cycles using standard methods
- **E-set選択 / E-set Selection**: バランスの取れたE-set選択戦略 / Balanced E-set selection strategy
- **局所探索 / Local Search**: 2-opt局所探索による改善 / Improvement with 2-opt local search

### 遺伝的アルゴリズムフレームワーク / Genetic Algorithm Framework

- **集団管理 / Population Management**: エリート保存戦略 / Elitist preservation strategy
- **親選択 / Parent Selection**: 適応度ベースの選択 / Fitness-based selection
- **多様性維持 / Diversity Maintenance**: 早期収束を防ぐメカニズム / Mechanisms to prevent premature convergence

## アルゴリズムの詳細 / Algorithm Details

### 初期化フェーズ / Initialization Phase

1. **TSPインスタンスの読み込み / Load TSP Instance**:
   ```
   - ファイルからデータを読み込み / Load data from file
   - 距離行列を計算 / Calculate distance matrix
   ```

2. **初期集団の生成 / Generate Initial Population**:
   ```
   - ランダムにツアーを生成 / Generate random tours
   - 各ツアーに2-optを適用 / Apply 2-opt to each tour
   - 適応度を評価 / Evaluate fitness
   ```

### 進化フェーズ / Evolution Phase

各世代で以下を実行:

For each generation:

1. **親選択 / Parent Selection**:
   - 2つの親を選択 / Select two parents
   - 適応度に基づく確率的選択 / Stochastic selection based on fitness

2. **EAX交叉 / EAX Crossover**:
   ```
   a. AB-cycleの構築:
      - 2つの親のツアーからE-setを構築
      - AB-cycleを特定
   
   b. E-setの選択:
      - AB-cycleのサブセットを選択
      - 複数の子個体を生成
   
   c. 最良の子を選択:
      - 生成された子の中から最良のものを選択
   ```

3. **局所探索 / Local Search**:
   - 2-opt法で子個体を改善 / Improve offspring with 2-opt

4. **集団更新 / Population Update**:
   - 子が親より良ければ置き換え / Replace parent if offspring is better
   - エリート個体を保持 / Preserve elite individuals

### 終了フェーズ / Termination Phase

- 指定世代数に到達 / Reach specified generations
- または収束を検出 / Or detect convergence
- 最良解を出力 / Output best solution

## アーキテクチャ / Architecture

### ソースコード構造 / Source Code Structure

```
normal_eax/
├── main.cpp                # エントリーポイント / Entry point
├── context.hpp             # コンテキスト管理 / Context management
├── context.cpp             # コンテキスト実装 / Context implementation
├── individual.hpp          # 個体クラス / Individual class
├── individual.cpp          # 個体実装 / Individual implementation
├── ga.hpp                  # GAメインループ / GA main loop
├── ga.cpp                  # GA実装 / GA implementation
└── generational_model.hpp  # 世代モデル / Generational model
```

### クラス設計 / Class Design

**Context (コンテキスト) / Context**:
- 実行環境の管理 / Manage execution environment
- グローバルパラメータの保持 / Hold global parameters
- リソースの管理 / Manage resources

**Individual (個体) / Individual**:
- ツアーの表現 / Tour representation
- 適応度の計算 / Fitness calculation
- 遺伝的操作 / Genetic operations

**GA (遺伝的アルゴリズム) / GA (Genetic Algorithm)**:
- 集団の管理 / Population management
- 進化ループの制御 / Control evolution loop
- 統計情報の収集 / Collect statistics

## パフォーマンス / Performance

### 実行時間の目安 / Execution Time Guidelines

問題サイズに基づく実行時間:

Execution time based on problem size:

| 都市数 / Cities | 集団サイズ / Pop Size | 世代数 / Gens | 時間 / Time |
|----------------|----------------------|--------------|------------|
| 100-300 | 100 | 500 | 1-3分 / 1-3 min |
| 300-500 | 150 | 1000 | 5-10分 / 5-10 min |
| 500-1000 | 200 | 1500 | 15-30分 / 15-30 min |

### メモリ使用量 / Memory Usage

集団サイズと問題サイズに依存:

Depends on population size and problem size:

- 小規模問題（<300都市）: < 100 MB
- 中規模問題（300-1000都市）: 100-500 MB
- 大規模問題（>1000都市）: > 500 MB

## Fast EAXとの比較 / Comparison with Fast EAX

| 特徴 / Feature | Normal EAX | Fast EAX |
|---------------|------------|----------|
| 速度 / Speed | 中速 / Medium | 高速 / Fast |
| 解の質 / Quality | 非常に良好 / Very Good | 良好 / Good |
| メモリ使用 / Memory | 標準 / Standard | 最適化 / Optimized |
| カスタマイズ性 / Customization | 標準 / Standard | 高い / High |
| 推奨用途 / Recommended Use | 一般的な問題 / General problems | 大規模・高速実行 / Large/Fast runs |

## 最適な使用シナリオ / Optimal Use Scenarios

### Normal EAXが適している場合 / When to Use Normal EAX

1. **中規模問題 / Medium-sized Problems**: 
   - 300-1000都市 / 300-1000 cities
   - バランスの取れた性能 / Balanced performance

2. **高品質解が必要 / High-Quality Solutions Needed**:
   - 解の質を重視 / Prioritize solution quality
   - 実行時間に余裕がある / Have time for execution

3. **標準的な実装が必要 / Standard Implementation Needed**:
   - 研究や比較のベースライン / Baseline for research or comparison
   - 教育目的 / Educational purposes

## チューニングガイド / Tuning Guide

### 集団サイズの選択 / Choosing Population Size

```
小規模問題: ps = 50-100
中規模問題: ps = 100-200
大規模問題: ps = 200-400
```

### 世代数の選択 / Choosing Number of Generations

```
探索的実行: 300-500世代
標準実行: 1000-2000世代
徹底的な最適化: 3000+世代
```

### 試行回数 / Number of Trials

複数試行を実行して最良解を見つける:
```bash
ARGS="--file problem.tsp --trials 10" make run/normal_eax
```

## デバッグとプロファイリング / Debugging and Profiling

### デバッグビルド / Debug Build

```bash
make bin/debug/normal_eax
gdb bin/debug/normal_eax
```

### プロファイリング / Profiling

```bash
make bin/prof/normal_eax
ARGS="--file problem.tsp" make prof_run/normal_eax
gprof bin/prof/normal_eax gmon.out > analysis.txt
```

## トラブルシューティング / Troubleshooting

### 問題: 収束が遅い / Problem: Slow Convergence

**解決策 / Solutions**:
- 集団サイズを増やす / Increase population size
- 異なるシード値を試す / Try different seed values
- 初期化方法を変更（貪欲法など） / Change initialization method (greedy, etc.)

### 問題: メモリ不足 / Problem: Out of Memory

**解決策 / Solutions**:
- 集団サイズを減らす / Reduce population size
- Fast EAXを使用（メモリ効率的） / Use Fast EAX (more memory efficient)

### 問題: 解の質が低い / Problem: Poor Solution Quality

**解決策 / Solutions**:
- 世代数を増やす / Increase number of generations
- 集団サイズを増やす / Increase population size
- 複数試行を実行 / Run multiple trials

## 関連項目 / See Also

- **[EAX](EAX)** - EAXアルゴリズムの概要 / Overview of EAX algorithms
- **[Fast EAX](Fast-EAX)** - 高速化バージョン / Optimized version
- **[EAX STSP](EAX-STSP)** - 対称TSP版 / Symmetric TSP version
- **[EAX Tabu](EAX-Tabu)** - Tabuサーチ版 / Tabu search version
- **[eaxlib](eaxlib)** - EAXライブラリのAPIリファレンス / EAX library API reference
