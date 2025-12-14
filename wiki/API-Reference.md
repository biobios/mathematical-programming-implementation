# API リファレンス / API Reference

## 概要 / Overview

このページでは、mathematical-programming-implementationプロジェクトの主要なAPIの詳細なリファレンスを提供します。

This page provides a detailed reference for the main APIs of the mathematical-programming-implementation project.

## 名前空間 / Namespaces

- `mpi::` - メイン名前空間 / Main namespace
- `mpi::linear_programming::` - 線形計画法 / Linear programming
- `mpi::eax::` - EAXアルゴリズム / EAX algorithm

## mpilib API

### Matrix クラス / Matrix Class

```cpp
namespace mpi {

template<std::size_t M, std::size_t N>
class Matrix {
public:
    // コンストラクタ / Constructors
    Matrix();
    Matrix(std::initializer_list<std::initializer_list<double>> values);
    Matrix(const Matrix& other);
    Matrix(Matrix&& other) noexcept;
    
    // 代入演算子 / Assignment operators
    Matrix& operator=(const Matrix& other);
    Matrix& operator=(Matrix&& other) noexcept;
    
    // 要素アクセス / Element access
    double& at(std::size_t i, std::size_t j);
    const double& at(std::size_t i, std::size_t j) const;
    double& operator()(std::size_t i, std::size_t j);
    const double& operator()(std::size_t i, std::size_t j) const;
    
    // サイズ情報 / Size information
    std::size_t rows() const { return M; }
    std::size_t cols() const { return N; }
    std::size_t size() const { return M * N; }
    
    // 転置 / Transpose
    Matrix<N, M> transpose() const;
    
    // 算術演算子 / Arithmetic operators
    Matrix operator+(const Matrix& other) const;
    Matrix operator-(const Matrix& other) const;
    Matrix operator*(double scalar) const;
    Matrix operator/(double scalar) const;
    
    template<std::size_t P>
    Matrix<M, P> operator*(const Matrix<N, P>& other) const;
    
    // 複合代入演算子 / Compound assignment operators
    Matrix& operator+=(const Matrix& other);
    Matrix& operator-=(const Matrix& other);
    Matrix& operator*=(double scalar);
    Matrix& operator/=(double scalar);
    
    // 比較演算子 / Comparison operators
    bool operator==(const Matrix& other) const;
    bool operator!=(const Matrix& other) const;
    
    // ユーティリティ / Utilities
    void fill(double value);
    void zero();
    Matrix<M, N> clone() const;
    
    // 反復子 / Iterators
    auto begin();
    auto end();
    auto begin() const;
    auto end() const;
};

// 非メンバ関数 / Non-member functions
template<std::size_t M, std::size_t N>
Matrix<M, N> operator*(double scalar, const Matrix<M, N>& mat);

template<std::size_t M, std::size_t N>
std::ostream& operator<<(std::ostream& os, const Matrix<M, N>& mat);

// ユーティリティ関数 / Utility functions
template<std::size_t N>
Matrix<N, N> identity();

template<std::size_t M, std::size_t N>
Matrix<M, N> zeros();

template<std::size_t M, std::size_t N>
Matrix<M, N> ones();

} // namespace mpi
```

#### 使用例 / Usage Examples

```cpp
// 行列の作成 / Create matrices
mpi::Matrix<2, 3> A = {{1, 2, 3}, {4, 5, 6}};
mpi::Matrix<3, 2> B = {{7, 8}, {9, 10}, {11, 12}};

// 行列演算 / Matrix operations
auto C = A * B;           // 2x2 行列 / 2x2 matrix
auto D = A.transpose();   // 3x2 行列 / 3x2 matrix
auto E = A * 2.0;         // スカラー倍 / Scalar multiplication

// 要素アクセス / Element access
double val = A.at(0, 1);  // val = 2
A.at(1, 2) = 100;         // A[1][2] = 100

// ユーティリティ / Utilities
auto I = mpi::identity<3>();  // 3x3 単位行列 / 3x3 identity matrix
auto Z = mpi::zeros<2, 3>();  // 2x3 零行列 / 2x3 zero matrix
```

### Simplex タブロー / Simplex Tableau

```cpp
namespace mpi::linear_programming {

enum class LPNoSolutionReason {
    Infeasible,  // 実行不可能 / Infeasible
    Unbounded    // 非有界 / Unbounded
};

class SimplexTableau {
public:
    SimplexTableau();
    
    // 線形計画問題を解く / Solve linear programming problem
    // minimize: c^T x
    // subject to: Ax = b, x >= 0
    template<std::size_t N, std::size_t M>
    std::expected<mpi::Matrix<N, 1>, LPNoSolutionReason>
    operator()(const mpi::Matrix<N, 1>& c,
               const mpi::Matrix<M, N>& A,
               const mpi::Matrix<M, 1>& b);
    
    // 詳細な結果を取得 / Get detailed results
    template<std::size_t N, std::size_t M>
    struct DetailedResult {
        mpi::Matrix<N, 1> solution;      // 最適解 / Optimal solution
        double objective_value;          // 目的関数値 / Objective value
        std::size_t iterations;          // 反復回数 / Number of iterations
        std::vector<std::size_t> basis;  // 基底変数 / Basis variables
    };
    
    template<std::size_t N, std::size_t M>
    std::expected<DetailedResult<N, M>, LPNoSolutionReason>
    solve_detailed(const mpi::Matrix<N, 1>& c,
                   const mpi::Matrix<M, N>& A,
                   const mpi::Matrix<M, 1>& b);
};

} // namespace mpi::linear_programming
```

#### 使用例 / Usage Examples

```cpp
// 線形計画問題を定義 / Define linear programming problem
// minimize: -2x1 - x2 - x3
// subject to: x1 + 2x2 = 12
//             x1 + 4x2 + 2x3 = 20
//             x1, x2, x3 >= 0

mpi::Matrix<3, 1> c = {{-2}, {-1}, {-1}};
mpi::Matrix<2, 3> A = {{1, 2, 0}, {1, 4, 2}};
mpi::Matrix<2, 1> b = {{12}, {20}};

// Simplexアルゴリズムで解く / Solve with Simplex algorithm
mpi::linear_programming::SimplexTableau simplex;
auto result = simplex(c, A, b);

if(result.has_value()) {
    auto x = result.value();
    std::cout << "Optimal solution found:\n";
    std::cout << "x1 = " << x.at(0, 0) << "\n";
    std::cout << "x2 = " << x.at(1, 0) << "\n";
    std::cout << "x3 = " << x.at(2, 0) << "\n";
    
    auto obj_val = c.transpose() * x;
    std::cout << "Objective value = " << obj_val.at(0, 0) << "\n";
} else {
    auto error = result.error();
    if(error == mpi::linear_programming::LPNoSolutionReason::Infeasible) {
        std::cout << "Problem is infeasible\n";
    } else {
        std::cout << "Problem is unbounded\n";
    }
}
```

### コマンドライン引数パーサー / Command-Line Argument Parser

```cpp
namespace mpi {

class ArgumentSpec {
public:
    // コンストラクタ / Constructors
    template<typename T>
    ArgumentSpec(T& variable);
    
    // 引数名の設定 / Set argument names
    void add_argument_name(const std::string& name);
    void add_set_argument_name(const std::string& name);
    void add_unset_argument_name(const std::string& name);
    
    // 説明文の設定 / Set description
    void set_description(const std::string& desc);
    
    // デフォルト値の設定 / Set default value
    template<typename T>
    void set_default(const T& default_value);
    
    // 必須フラグの設定 / Set required flag
    void set_required(bool required = true);
};

class CommandLineArgumentParser {
public:
    CommandLineArgumentParser();
    
    // 引数仕様を追加 / Add argument specification
    void add_argument(ArgumentSpec spec);
    void add_argument(ArgumentSpec&& spec);
    
    // 引数を解析 / Parse arguments
    void parse(int argc, char* argv[]);
    void parse(const std::vector<std::string>& args);
    
    // ヘルプを表示 / Print help
    void print_help() const;
    void print_help(std::ostream& os) const;
    
    // バージョンを設定 / Set version
    void set_version(const std::string& version);
    
    // プログラム名を設定 / Set program name
    void set_program_name(const std::string& name);
};

} // namespace mpi
```

#### 使用例 / Usage Examples

```cpp
int main(int argc, char* argv[]) {
    // パラメータ変数 / Parameter variables
    std::string input_file;
    int population_size = 100;
    double mutation_rate = 0.01;
    bool verbose = false;
    
    // パーサーを作成 / Create parser
    mpi::CommandLineArgumentParser parser;
    parser.set_program_name("my_algorithm");
    parser.set_version("1.0.0");
    
    // ファイル引数 / File argument
    mpi::ArgumentSpec file_spec(input_file);
    file_spec.add_argument_name("--file");
    file_spec.add_argument_name("-f");
    file_spec.set_description("Input file path");
    file_spec.set_required(true);
    parser.add_argument(std::move(file_spec));
    
    // 集団サイズ引数 / Population size argument
    mpi::ArgumentSpec ps_spec(population_size);
    ps_spec.add_argument_name("--population-size");
    ps_spec.add_argument_name("-p");
    ps_spec.set_description("Population size (default: 100)");
    parser.add_argument(std::move(ps_spec));
    
    // 突然変異率引数 / Mutation rate argument
    mpi::ArgumentSpec mr_spec(mutation_rate);
    mr_spec.add_argument_name("--mutation-rate");
    mr_spec.add_argument_name("-m");
    mr_spec.set_description("Mutation rate (default: 0.01)");
    parser.add_argument(std::move(mr_spec));
    
    // 詳細出力フラグ / Verbose flag
    mpi::ArgumentSpec verbose_spec(verbose);
    verbose_spec.add_set_argument_name("--verbose");
    verbose_spec.add_set_argument_name("-v");
    verbose_spec.set_description("Enable verbose output");
    parser.add_argument(std::move(verbose_spec));
    
    // ヘルプフラグ / Help flag
    bool show_help = false;
    mpi::ArgumentSpec help_spec(show_help);
    help_spec.add_set_argument_name("--help");
    help_spec.add_set_argument_name("-h");
    help_spec.set_description("Show this help message");
    parser.add_argument(std::move(help_spec));
    
    // 引数を解析 / Parse arguments
    try {
        parser.parse(argc, argv);
    } catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        parser.print_help();
        return 1;
    }
    
    if(show_help) {
        parser.print_help();
        return 0;
    }
    
    // パラメータを使用 / Use parameters
    std::cout << "Input file: " << input_file << "\n";
    std::cout << "Population size: " << population_size << "\n";
    std::cout << "Mutation rate: " << mutation_rate << "\n";
    std::cout << "Verbose: " << (verbose ? "yes" : "no") << "\n";
    
    return 0;
}
```

## eaxlib API

### EAX 交叉オペレータ / EAX Crossover Operator

```cpp
namespace mpi::eax {

class EAXNormal {
public:
    using DistanceMatrix = std::vector<std::vector<int64_t>>;
    using Tour = std::vector<std::array<std::size_t, 2>>;
    
    // コンストラクタ / Constructor
    explicit EAXNormal(const DistanceMatrix& dist_matrix);
    
    // 交叉を実行 / Perform crossover
    Tour crossover(const Tour& parent1, const Tour& parent2);
    
    // パラメータの設定 / Set parameters
    void set_num_children(std::size_t num);
    void set_e_set_selection_method(ESetSelectionMethod method);
    
    // 統計情報を取得 / Get statistics
    struct Statistics {
        std::size_t num_ab_cycles;
        std::size_t num_e_sets_generated;
        std::size_t num_valid_offspring;
        double average_offspring_quality;
    };
    
    Statistics get_statistics() const;
};

class EAXNAb {
public:
    using DistanceMatrix = std::vector<std::vector<int64_t>>;
    using Tour = std::vector<std::array<std::size_t, 2>>;
    
    // コンストラクタ / Constructor
    EAXNAb(const DistanceMatrix& dist_matrix, std::size_t num_ab_cycles);
    
    // 交叉を実行 / Perform crossover
    Tour crossover(const Tour& parent1, const Tour& parent2);
    
    // AB-cycleの数を設定 / Set number of AB-cycles
    void set_num_ab_cycles(std::size_t num);
};

} // namespace mpi::eax
```

### AB-cycle 関連 / AB-cycle Related

```cpp
namespace mpi::eax {

struct Edge {
    std::size_t from;
    std::size_t to;
    
    bool operator==(const Edge& other) const;
    bool operator!=(const Edge& other) const;
};

class ABCycle {
public:
    ABCycle();
    
    // エッジを追加 / Add edge
    void add_edge(const Edge& edge);
    
    // AB-cycleの情報 / AB-cycle information
    std::size_t length() const;
    const std::vector<Edge>& edges() const;
    bool is_closed() const;
    
    // イテレータ / Iterators
    auto begin() const { return edges_.begin(); }
    auto end() const { return edges_.end(); }
    
private:
    std::vector<Edge> edges_;
};

class ABCycleBuilder {
public:
    using Tour = std::vector<std::array<std::size_t, 2>>;
    using DistanceMatrix = std::vector<std::vector<int64_t>>;
    
    ABCycleBuilder();
    
    // AB-cycleを構築 / Build AB-cycles
    std::vector<ABCycle> build(const Tour& parent1,
                                const Tour& parent2,
                                const DistanceMatrix& dist_matrix);
    
    // ビルダーの設定 / Builder settings
    void set_max_ab_cycles(std::size_t max);
};

} // namespace mpi::eax
```

### E-set 関連 / E-set Related

```cpp
namespace mpi::eax {

class ESet {
public:
    ESet();
    
    // エッジの管理 / Edge management
    void add_edge(const Edge& edge);
    void remove_edge(const Edge& edge);
    bool contains(const Edge& edge) const;
    
    // E-setの情報 / E-set information
    std::size_t size() const;
    const std::vector<Edge>& edges() const;
    
    // E-setを適用 / Apply E-set
    Tour apply(const Tour& parent) const;
    
    // イテレータ / Iterators
    auto begin() const { return edges_.begin(); }
    auto end() const { return edges_.end(); }
    
private:
    std::vector<Edge> edges_;
    std::unordered_set<Edge> edge_set_;
};

class Block2ESetAssembler {
public:
    using Tour = std::vector<std::array<std::size_t, 2>>;
    
    Block2ESetAssembler();
    
    // E-setを組み立て / Assemble E-sets
    std::vector<ESet> assemble(const std::vector<ABCycle>& ab_cycles,
                                const Tour& parent1,
                                const Tour& parent2);
    
    // アセンブラーの設定 / Assembler settings
    void set_max_e_sets(std::size_t max);
};

} // namespace mpi::eax
```

### オブジェクトプール / Object Pools

```cpp
namespace mpi::eax {

class ObjectPools {
public:
    // コンストラクタ / Constructor
    explicit ObjectPools(std::size_t max_cities);
    
    // オブジェクトの取得 / Acquire object
    template<typename T>
    T* acquire();
    
    // オブジェクトの返却 / Release object
    template<typename T>
    void release(T* obj);
    
    // プールをクリア / Clear pool
    void clear();
    
    // 統計情報 / Statistics
    std::size_t get_pool_size() const;
    std::size_t get_active_objects() const;
};

} // namespace mpi::eax
```

## ユーティリティ関数 / Utility Functions

### TSP 関連 / TSP Related

```cpp
namespace mpi {

// TSPファイルを読み込み / Load TSP file
struct TSPData {
    std::string name;
    std::size_t dimension;
    std::string edge_weight_type;
    std::vector<std::array<double, 2>> coordinates;
    std::vector<std::vector<int64_t>> distance_matrix;
};

TSPData load_tsp_file(const std::string& filename);

// ツアー長を計算 / Calculate tour length
double calculate_tour_length(const std::vector<std::size_t>& tour,
                             const std::vector<std::vector<int64_t>>& dist_matrix);

// ツアーの有効性を確認 / Verify tour validity
bool is_valid_tsp_tour(const std::vector<std::size_t>& tour,
                       std::size_t num_cities);

} // namespace mpi
```

### 2-opt 局所探索 / 2-opt Local Search

```cpp
namespace mpi {

// 2-optでツアーを改善 / Improve tour with 2-opt
void two_opt_improvement(std::vector<std::size_t>& tour,
                        const std::vector<std::vector<int64_t>>& dist_matrix);

// 近傍2-opt / Neighbor 2-opt
void neighbor_two_opt(std::vector<std::size_t>& tour,
                     const std::vector<std::vector<int64_t>>& dist_matrix,
                     std::size_t neighbor_radius);

// 2-opt移動の評価 / Evaluate 2-opt move
int64_t evaluate_two_opt_move(const std::vector<std::size_t>& tour,
                               const std::vector<std::vector<int64_t>>& dist_matrix,
                               std::size_t i,
                               std::size_t j);

} // namespace mpi
```

## 型エイリアス / Type Aliases

```cpp
namespace mpi {

// 距離行列の型 / Distance matrix type
using DistanceMatrix = std::vector<std::vector<int64_t>>;

// ツアーの型（経路形式） / Tour type (path format)
using TourPath = std::vector<std::size_t>;

// ツアーの型（隣接リスト形式） / Tour type (adjacency list format)
using TourAdjacency = std::vector<std::array<std::size_t, 2>>;

// 座標の型 / Coordinate type
using Coordinate = std::array<double, 2>;
using Coordinates = std::vector<Coordinate>;

} // namespace mpi
```

## 定数 / Constants

```cpp
namespace mpi {

// デフォルト値 / Default values
constexpr std::size_t DEFAULT_POPULATION_SIZE = 100;
constexpr std::size_t DEFAULT_GENERATIONS = 300;
constexpr double DEFAULT_MUTATION_RATE = 0.01;
constexpr std::size_t DEFAULT_TOURNAMENT_SIZE = 3;

} // namespace mpi

namespace mpi::eax {

// EAXデフォルト値 / EAX default values
constexpr std::size_t DEFAULT_NUM_CHILDREN = 10;
constexpr std::size_t DEFAULT_MAX_AB_CYCLES = 100;
constexpr std::size_t DEFAULT_MAX_E_SETS = 50;

} // namespace mpi::eax
```

## エラー処理 / Error Handling

### 例外クラス / Exception Classes

```cpp
namespace mpi {

class MPIException : public std::exception {
public:
    explicit MPIException(const std::string& message);
    const char* what() const noexcept override;
};

class InvalidArgumentException : public MPIException {
public:
    explicit InvalidArgumentException(const std::string& message);
};

class FileNotFoundException : public MPIException {
public:
    explicit FileNotFoundException(const std::string& filename);
};

class InvalidTSPFileException : public MPIException {
public:
    explicit InvalidTSPFileException(const std::string& message);
};

class InvalidTourException : public MPIException {
public:
    explicit InvalidTourException(const std::string& message);
};

} // namespace mpi
```

## 関連項目 / See Also

- **[Getting Started](Getting-Started)** - はじめに / Getting started guide
- **[mpilib](mpilib)** - mpilibの詳細 / mpilib details
- **[eaxlib](eaxlib)** - eaxlibの詳細 / eaxlib details
- **[Simplex](Simplex)** - Simplexアルゴリズムの使用方法 / How to use Simplex algorithm
- **[EAX](EAX)** - EAXアルゴリズムの使用方法 / How to use EAX algorithm
