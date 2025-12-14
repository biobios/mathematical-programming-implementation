# ビルド手順 / Build Instructions

## ビルドシステムの概要 / Build System Overview

このプロジェクトは、階層的なMakefileシステムを使用しています。ルートディレクトリのMakefileが各サブプロジェクトのビルドを調整します。

This project uses a hierarchical Makefile system. The root Makefile coordinates the build of each subproject.

## コンパイラフラグ / Compiler Flags

デフォルトのコンパイラフラグ / Default compiler flags:
```
-std=c++23          # C++23標準を使用 / Use C++23 standard
-O3                 # 最適化レベル3 / Optimization level 3
-Wall               # すべての警告を有効化 / Enable all warnings
-Wextra             # 追加の警告を有効化 / Enable extra warnings
-pedantic           # 厳格な標準準拠 / Strict standard compliance
-mtune=native       # CPUに最適化 / Optimize for CPU
-march=native       # CPU固有の命令を使用 / Use CPU-specific instructions
-flto               # リンク時最適化 / Link-time optimization
```

## プロジェクト構造 / Project Structure

### アプリケーションプロジェクト / Application Projects
- `eax` - ATSPのためのEAX / EAX for ATSP
- `eax_stsp` - STSPのためのEAX / EAX for STSP
- `eax_tabu` - Tabuサーチ付きEAX / EAX with Tabu search
- `fast_eax` - 高速EAX / Fast EAX
- `normal_eax` - 標準EAX / Normal EAX
- `simplex` - Simplexアルゴリズム / Simplex algorithm

### ライブラリプロジェクト / Library Projects
- `libmpilib` - 数理計画法ライブラリ / Mathematical programming library
- `libeaxlib` - EAXライブラリ / EAX library

## ビルドターゲット / Build Targets

### すべてをビルド / Build Everything
```bash
make all
```

### 特定のアプリケーションをビルド / Build Specific Applications
```bash
make bin/simplex
make bin/eax
make bin/eax_stsp
make bin/fast_eax
make bin/normal_eax
make bin/eax_tabu
```

### ライブラリをビルド / Build Libraries
```bash
make bin/libmpilib.a
make bin/libeaxlib.a
```

### デバッグビルド / Debug Builds
デバッグシンボル（`-g`）付きでコンパイル / Compile with debug symbols (`-g`):
```bash
make bin/debug/simplex
make bin/debug/eax_stsp
make bin/debug/libmpilib.a
```

### プロファイリングビルド / Profiling Builds
プロファイリングサポート（`-pg`）付きでコンパイル / Compile with profiling support (`-pg`):
```bash
make bin/prof/simplex
make bin/prof/eax_stsp
make bin/prof/libmpilib.a
```

## 実行ターゲット / Run Targets

### 通常実行 / Normal Execution
```bash
make run/simplex
make run/eax
make run/eax_stsp
make run/fast_eax
```

### 引数付き実行 / Execution with Arguments
```bash
ARGS="--file data/eax_stsp/att532.tsp --ps 100" make run/eax_stsp
ARGS="--help" make run/fast_eax
```

### プロファイリング実行 / Profiling Execution
```bash
make prof_run/simplex
ARGS="--file data.tsp" make prof_run/eax_stsp
```

## ビルド成果物の構造 / Build Artifacts Structure

```
bin/
├── simplex              # 通常ビルド / Normal builds
├── eax
├── eax_stsp
├── fast_eax
├── normal_eax
├── eax_tabu
├── libmpilib.a
├── libeaxlib.a
├── debug/              # デバッグビルド / Debug builds
│   ├── simplex
│   ├── eax_stsp
│   └── ...
└── prof/               # プロファイリングビルド / Profiling builds
    ├── simplex
    ├── eax_stsp
    └── ...
```

## クリーンアップ / Cleanup

すべてのビルド成果物を削除 / Remove all build artifacts:
```bash
make clean
```

これにより以下が削除されます / This removes:
- `temp/` ディレクトリ / directory
- `bin/` ディレクトリ / directory
- `debug/` ディレクトリ / directory
- 各プロジェクトの`Makefile`

## トラブルシューティング / Troubleshooting

### コンパイラが見つからない / Compiler Not Found
C++23をサポートするコンパイラがインストールされていることを確認してください。

Ensure you have a compiler that supports C++23 installed:
```bash
g++ --version    # Should be 11 or higher
clang++ --version # Should be 14 or higher
```

### ビルドエラー / Build Errors
1. `make setup`を実行してMakefileが生成されていることを確認 / Run `make setup` to ensure Makefiles are generated
2. 依存関係を確認 / Check dependencies
3. `make clean`を実行してからリビルド / Run `make clean` and rebuild

### リンクエラー / Link Errors
ライブラリが最初にビルドされていることを確認 / Ensure libraries are built first:
```bash
make bin/libmpilib.a
make bin/libeaxlib.a
```

## カスタマイズ / Customization

### コンパイラフラグの追加 / Adding Custom Compiler Flags
環境変数`CXXFLAGS`を使用 / Use the `CXXFLAGS` environment variable:
```bash
CXXFLAGS="-DDEBUG -DVERBOSE" make bin/simplex
```

### 異なるコンパイラの使用 / Using a Different Compiler
```bash
CXX=clang++ make bin/simplex
```
