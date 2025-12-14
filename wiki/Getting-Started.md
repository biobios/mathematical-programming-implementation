# はじめに / Getting Started

## 必要な環境 / Requirements

- C++23対応のコンパイラ（GCC 11以降、Clang 14以降を推奨）/ C++23-compatible compiler (GCC 11+ or Clang 14+ recommended)
- Make
- Git

## インストール / Installation

### 1. リポジトリのクローン / Clone the Repository

```bash
git clone https://github.com/biobios/mathematical-programming-implementation.git
cd mathematical-programming-implementation
```

### 2. Makefileのセットアップ / Setup Makefiles

```bash
make setup
```

このコマンドは、各プロジェクトディレクトリにMakefileを生成します。

This command generates Makefiles in each project directory.

### 3. プロジェクトのビルド / Build a Project

すべてのプロジェクトをビルド / Build all projects:
```bash
make all
```

特定のプロジェクトをビルド / Build a specific project:
```bash
make bin/simplex        # Simplexアルゴリズム / Simplex algorithm
make bin/eax_stsp       # EAX for STSP
make bin/fast_eax       # Fast EAX
```

## 基本的な使い方 / Basic Usage

### Simplexアルゴリズムの実行 / Running the Simplex Algorithm

```bash
make run/simplex
```

このコマンドは、ハードコードされた線形計画問題を解きます。

This command solves a hardcoded linear programming problem.

### EAXアルゴリズムの実行 / Running the EAX Algorithm

```bash
# EAX for STSP
ARGS="--file data/eax_stsp/att532.tsp --ps 100 --generations 300" make run/eax_stsp

# Fast EAX
ARGS="--file data/fast_eax/att532.tsp --ps 100" make run/fast_eax
```

利用可能な引数を確認 / Check available arguments:
```bash
ARGS="--help" make run/eax_stsp
ARGS="--help" make run/fast_eax
```

## デバッグビルド / Debug Build

デバッグシンボル付きでビルド / Build with debug symbols:
```bash
make bin/debug/simplex
make bin/debug/eax_stsp
```

## プロファイリングビルド / Profiling Build

プロファイリング情報付きでビルド / Build with profiling information:
```bash
make bin/prof/simplex
make bin/prof/eax_stsp
```

プロファイリング実行 / Run with profiling:
```bash
make prof_run/simplex
ARGS="--file data/eax_stsp/att532.tsp" make prof_run/eax_stsp
```

## クリーンアップ / Cleanup

すべてのビルド成果物を削除 / Remove all build artifacts:
```bash
make clean
```

## 次のステップ / Next Steps

- **[ビルド手順](Build-Instructions)** - ビルドシステムの詳細 / Build system details
- **[Simplex](Simplex)** - Simplexアルゴリズムの使い方 / How to use the Simplex algorithm
- **[EAX](EAX)** - EAXアルゴリズムの使い方 / How to use the EAX algorithm
- **[API リファレンス](API-Reference)** - APIドキュメント / API documentation
