# Simplex アルゴリズム / Simplex Algorithm

## 概要 / Overview

Simplexアルゴリズムは、線形計画問題を解くための古典的なアルゴリズムです。このプロジェクトでは、行列ベースの実装を提供しています。

The Simplex algorithm is a classical algorithm for solving linear programming problems. This project provides a matrix-based implementation.

## 線形計画問題 / Linear Programming Problem

線形計画問題は以下の形式で表されます:

A linear programming problem is expressed in the following form:

```
minimize:    c^T x
subject to:  Ax = b
             x ≥ 0
```

ここで:
- `c` は目的関数の係数ベクトル / objective function coefficient vector
- `A` は制約行列 / constraint matrix
- `b` は制約の右辺ベクトル / right-hand side constraint vector
- `x` は決定変数ベクトル / decision variable vector

## ビルド / Build

```bash
# 通常ビルド / Normal build
make bin/simplex

# デバッグビルド / Debug build
make bin/debug/simplex

# プロファイリングビルド / Profiling build
make bin/prof/simplex
```

## 使い方 / Usage

### 基本的な実行 / Basic Execution

```bash
make run/simplex
```

### プログラムの内容 / Program Contents

現在の実装では、以下の線形計画問題を解きます:

The current implementation solves the following linear programming problem:

```
minimize:    -2x₁ - x₂ - x₃
subject to:  x₁ + 2x₂     = 12
             x₁ + 4x₂ + 2x₃ = 20
             x₁, x₂, x₃ ≥ 0
```

## API 使用例 / API Usage Example

```cpp
#include "matrix.hpp"
#include "simplex.hpp"

int main()
{
    // 目的関数の係数 / Objective function coefficients
    mpi::Matrix<3, 1> c = {{-2}, {-1}, {-1}};
    
    // 制約行列 / Constraint matrix
    mpi::Matrix<2, 3> A = {{1, 2, 0}, {1, 4, 2}};
    
    // 右辺ベクトル / Right-hand side vector
    mpi::Matrix<2, 1> b = {{12}, {20}};

    // Simplexアルゴリズムを実行 / Run Simplex algorithm
    auto result = mpi::linear_programming::SimplexTableau()(c, A, b);
    
    if(result.has_value())
    {
        std::cout << "Optimal solution found" << std::endl;
        std::cout << "x = ";
        for (std::size_t i = 0; i < 3; ++i)
        {
            std::cout << result.value().at(i, 0) << " ";
        }
        std::cout << std::endl;

        // 目的関数値を計算 / Calculate objective value
        auto objective_value = c.transpose() * result.value();
        std::cout << "Objective value = " << objective_value << std::endl;
    }
    else
    {
        // エラー処理 / Error handling
        if(result.error() == mpi::linear_programming::LPNoSolutionReason::Infeasible)
        {
            std::cout << "Problem is infeasible" << std::endl;
        }
        else if(result.error() == mpi::linear_programming::LPNoSolutionReason::Unbounded)
        {
            std::cout << "Problem is unbounded" << std::endl;
        }
    }

    return 0;
}
```

## クラスとメソッド / Classes and Methods

### `mpi::linear_programming::SimplexTableau`

線形計画問題を解くためのSimplexタブローを表すクラス。

Class representing the Simplex tableau for solving linear programming problems.

#### メソッド / Methods

```cpp
template<std::size_t N, std::size_t M>
std::expected<mpi::Matrix<N, 1>, LPNoSolutionReason> 
operator()(const mpi::Matrix<N, 1>& c, 
           const mpi::Matrix<M, N>& A, 
           const mpi::Matrix<M, 1>& b)
```

**パラメータ / Parameters:**
- `c`: 目的関数の係数ベクトル (サイズ N×1) / Objective function coefficient vector (size N×1)
- `A`: 制約行列 (サイズ M×N) / Constraint matrix (size M×N)
- `b`: 右辺ベクトル (サイズ M×1) / Right-hand side vector (size M×1)

**戻り値 / Returns:**
- 成功時: 最適解ベクトル `x` (サイズ N×1) / On success: optimal solution vector `x` (size N×1)
- 失敗時: エラー理由 / On failure: error reason
  - `LPNoSolutionReason::Infeasible` - 実行不可能 / Infeasible
  - `LPNoSolutionReason::Unbounded` - 非有界 / Unbounded

## エラー処理 / Error Handling

### `LPNoSolutionReason`

```cpp
enum class LPNoSolutionReason
{
    Infeasible,  // 実行可能解が存在しない / No feasible solution exists
    Unbounded    // 目的関数が非有界 / Objective function is unbounded
};
```

## アルゴリズムの詳細 / Algorithm Details

1. **初期化 / Initialization**: 
   - スラック変数を追加して標準形に変換 / Add slack variables to convert to standard form
   - 初期基底解を設定 / Set initial basic feasible solution

2. **反復 / Iteration**:
   - ピボット列を選択（最も負の被約費用） / Select pivot column (most negative reduced cost)
   - ピボット行を選択（最小比テスト） / Select pivot row (minimum ratio test)
   - ピボット操作を実行 / Perform pivot operation

3. **終了条件 / Termination**:
   - すべての被約費用が非負 → 最適解 / All reduced costs non-negative → optimal solution
   - 非有界 → 問題が非有界 / Unbounded → problem is unbounded
   - 実行不可能 → 実行可能解なし / Infeasible → no feasible solution

## 制限事項 / Limitations

- 現在の実装は、等式制約のみをサポート / Current implementation supports equality constraints only
- 不等式制約を使用する場合は、スラック変数を手動で追加する必要があります / For inequality constraints, you must manually add slack variables
- 退化の処理は最小限です / Degeneracy handling is minimal

## パフォーマンス / Performance

- コンパイル時に行列サイズが固定されるため、最適化が容易 / Matrix sizes are fixed at compile time, enabling better optimization
- テンプレートベースの実装により、ランタイムのオーバーヘッドを削減 / Template-based implementation reduces runtime overhead
- `-O3 -flto`フラグで最適化 / Optimized with `-O3 -flto` flags

## 参考文献 / References

- Dantzig, G. B. (1951). "Maximization of a linear function of variables subject to linear inequalities."
- Chvátal, V. (1983). "Linear Programming." W. H. Freeman.

## 関連項目 / See Also

- **[mpilib](mpilib)** - 行列クラスと数理計画法ユーティリティ / Matrix class and mathematical programming utilities
- **[API Reference](API-Reference)** - 完全なAPIドキュメント / Complete API documentation
