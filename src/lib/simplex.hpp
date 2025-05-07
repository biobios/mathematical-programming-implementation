#pragma once

#include <expected>

#include "matrix.hpp"
#include "ranges_fixed_size.hpp"
#include "linear_programming.hpp"
#include "submatrix.hpp"

namespace mpi
{
    namespace linear_programming
    {
        struct Simplex
        {
            template <std::size_t NumVariables, std::size_t NumConstraints, typename ValueType>
            constexpr std::expected<Matrix<NumVariables, 1, ValueType>, LPNoSolutionReason> operator()(const Matrix<NumVariables, 1, ValueType> &objective_function_coefficients,
                                                                                             const Matrix<NumConstraints, NumVariables, ValueType> &constraints_coefficients,
                                                                                             const Matrix<NumConstraints, 1, ValueType> &constraint_values) const
            {
                // 2段階シンプレックス法
                // 補助問題の行列を作成する
                Matrix<NumVariables + NumConstraints, 1, ValueType> auxiliary_objective_function_coefficients;
                Matrix<NumConstraints, NumVariables + NumConstraints, ValueType> auxiliary_constraints_coefficients;
                // 目的関数の係数を作成（人為変数の係数のみ1）
                for (std::size_t i = NumVariables; i < NumVariables + NumConstraints; ++i)
                {
                    auxiliary_objective_function_coefficients.at(i, 0) = 1;
                }
                // 制約条件の係数をコピー
                for (std::size_t i = 0; i < NumConstraints; ++i)
                {
                    for (std::size_t j = 0; j < NumVariables; ++j)
                    {
                        auxiliary_constraints_coefficients.at(i, j) = constraints_coefficients.at(i, j);
                    }
                }
                // 人為変数の係数を追加
                for (std::size_t i = 0; i < NumConstraints; ++i)
                {
                    // 制約条件の値が負の場合は、係数は-1
                    if (constraint_values.at(i, 0) < 0)
                    {
                        auxiliary_constraints_coefficients.at(i, NumVariables + i) = -1;
                    }else
                    {
                        auxiliary_constraints_coefficients.at(i, NumVariables + i) = 1;
                    }
                }
                std::array<std::size_t, NumVariables + NumConstraints> indices_of_first_basis_solution;
                // 人為変数を基底変数に指定
                for (std::size_t i = 0; i < NumConstraints; ++i)
                {
                    indices_of_first_basis_solution.at(i) = NumVariables + i;
                }
                // 元の変数を非基底変数に指定
                for (std::size_t i = 0; i < NumVariables; ++i)
                {
                    indices_of_first_basis_solution.at(NumConstraints + i) = i;
                }
                
                // 補助問題を解く
                auto result = solve_with_feasible_basis_solution(auxiliary_objective_function_coefficients, auxiliary_constraints_coefficients, constraint_values, indices_of_first_basis_solution);
                
                // 元の問題の初期実行可能基底解を決める
                std::array<std::size_t, NumVariables> indices_of_first_basis_solution_for_original_problem;
                if (!result.has_value())
                {
                    return std::unexpected(result.error());
                }else
                {
                    // 人為変数の値がすべて0でなければ、元の問題は実行不可能
                    for (std::size_t i = 0; i < NumConstraints; ++i)
                    {
                        if (result.value().at(NumVariables + i, 0) != 0)
                        {
                            return std::unexpected(LPNoSolutionReason::Infeasible);
                        }
                    }

                    std::size_t i_base_var = 0;
                    std::size_t i_non_base_var = 0;

                    for (std::size_t i = 0; i < NumVariables; ++i)
                    {
                        if (result.value().at(i, 0) != 0)
                        {
                            // 基底変数にする
                            indices_of_first_basis_solution_for_original_problem[i_base_var] = i;
                            ++i_base_var;
                        }else
                        {
                            // 非基底変数にする
                            indices_of_first_basis_solution_for_original_problem[NumConstraints + i_non_base_var] = i;
                            ++i_non_base_var;
                        }
                    }
                }
                
                // 元の問題を解く
                return solve_with_feasible_basis_solution(objective_function_coefficients, constraints_coefficients, constraint_values, indices_of_first_basis_solution_for_original_problem);
            }

        private:
            template <std::size_t Rows, typename ValueType>
            static constexpr bool is_feasible_basis_solution(const mpi::Matrix<Rows, 1, ValueType>& solution)
            {
                for (std::size_t i = 0; i < Rows; ++i)
                {
                    if (solution.at(i, 0) < 0)
                    {
                        return false;
                    }
                }
                return true;
            }
            
            
            template <std::size_t NumVariables, std::size_t NumConstraints, typename ValueType>
            static constexpr std::expected<Matrix<NumVariables, 1, ValueType>, LPNoSolutionReason> solve_with_feasible_basis_solution(const Matrix<NumVariables, 1, ValueType> &objective_function_coefficients,
                                                                                             const Matrix<NumConstraints, NumVariables, ValueType> &constraints_coefficients,
                                                                                             const Matrix<NumConstraints, 1, ValueType> &constraint_values, const std::array<std::size_t, NumVariables>& indices_of_first_basis_solution)
            {
                // 基底変数の個数
                constexpr std::size_t num_base_variables = NumConstraints;
                // 非基底変数の個数
                constexpr std::size_t num_non_base_variables = NumVariables - num_base_variables;

                auto& b = constraint_values;

                // 0 ~ num_base_variables-1まで基底変数のインデックス
                // num_base_variables ~ num_variables-1まで非基底変数のインデックス
                std::array<std::size_t, NumVariables> indices = indices_of_first_basis_solution;

                // (0) 初期実行可能基底解を選ぶ

                auto B = create_submatrix_selecting(constraints_coefficients, {}, ranges::fixed_size<num_base_variables>(0)(indices));
                auto B_inv = B.calc_inverse();
                auto b_bar = B_inv * b;

                // 与えられた基底解が実行不可能だったら、終了する
                if (!is_feasible_basis_solution(b_bar)){
                    return std::unexpected(LPNoSolutionReason::Infeasible);
                }

                while(true){

                    auto base_variable_indices = ranges::fixed_size<num_base_variables>(0)(indices);
                    auto non_base_variable_indices = ranges::fixed_size<num_non_base_variables>(num_base_variables)(indices);

                    auto B = create_submatrix_selecting(constraints_coefficients, {}, base_variable_indices);
                    auto N = create_submatrix_selecting(constraints_coefficients, {}, non_base_variable_indices);
                    auto B_inv = B.calc_inverse();
                    auto c_B = create_submatrix_selecting(objective_function_coefficients, base_variable_indices);
                    auto c_N = create_submatrix_selecting(objective_function_coefficients, non_base_variable_indices);
                    auto b_bar = B_inv * b;

                    // (1) シンプレックス乗数を計算する

                    auto pi = B_inv.transpose() * c_B;

                    // (2) 相対コスト係数が全て非負であれば終了
                    //     相対コスト係数が負のものがあれば、最も負のものを選ぶ

                    auto relative_cost = c_N - N.transpose() * pi;

                    bool is_optimal = true;
                    ValueType min = 0;
                    std::size_t x_k_index = 0;
                    for (std::size_t i = 0; i < num_non_base_variables; ++i)
                    {
                        auto cost = relative_cost.at(i, 0);
                        if (cost < min)
                        {
                            min = cost;
                            x_k_index = num_base_variables + i;
                            is_optimal = false;
                        }
                    }

                    if(is_optimal)
                    {
                        Matrix<NumVariables, 1, ValueType> result;
                        for (std::size_t b_i = 0; auto r_i : base_variable_indices)
                        {
                            result.at(r_i, 0) = b_bar.at(b_i, 0);
                            ++b_i;
                        }

                        return result;
                    }

                    // (3) ベクトルyを計算する
                    
                    auto y = B_inv * create_submatrix_selecting(constraints_coefficients, {}, ranges::fixed_size<1>(x_k_index)(indices));

                    // (4) yに正の要素が無ければ、有界でないので終了
                    //     yに正の要素があれば、それらの中で \bar b_i / y_i = θ となるiを求める

                    bool has_positive = false;
                    ValueType theta = std::numeric_limits<ValueType>::max();
                    std::size_t i_index = 0;
                    for (std::size_t i = 0; i < num_base_variables; ++i)
                    {
                        if (y.at(i, 0) > 0)
                        {
                            has_positive = true;
                            auto theta_i = b_bar.at(i, 0) / y.at(i, 0);
                            if (theta_i < theta)
                            {
                                theta = theta_i;
                                i_index = i;
                            }
                        }
                    }

                    if (!has_positive)
                    {
                        return std::unexpected(LPNoSolutionReason::Unbounded);
                    }

                    // (5) x_kを基底変数にして、iに対応するxを非基底変数にして、(1)に戻る
                    std::swap(indices[i_index], indices[x_k_index]);
                }
            }
        };
        
        struct SimplexTableau
        {

            template <std::size_t NumVariables, std::size_t NumConstraints, typename ValueType>
            constexpr std::expected<Matrix<NumVariables, 1, ValueType>, LPNoSolutionReason> operator()(const Matrix<NumVariables, 1, ValueType> &objective_function_coefficients,
                                                                                             const Matrix<NumConstraints, NumVariables, ValueType> &constraints_coefficients,
                                                                                             const Matrix<NumConstraints, 1, ValueType> &constraint_values) const
            {
                // 補助問題を解く
                Matrix<NumConstraints + 1, NumVariables + NumConstraints + 1, ValueType> auxiliary_tableau;
                // 目的関数の係数を作成（人為変数の係数のみ1）
                for (std::size_t i = 0; i < NumConstraints; ++i)
                {
                    auxiliary_tableau.at(0, NumVariables + i) = 1;
                }
                // 制約条件の係数をコピー
                for (std::size_t i = 0; i < NumConstraints; ++i)
                {
                    for (std::size_t j = 0; j < NumVariables; ++j)
                    {
                        auxiliary_tableau.at(i + 1, j) = constraints_coefficients.at(i, j);
                    }
                    // 制約条件の値をコピー
                    auxiliary_tableau.at(i + 1, NumVariables + NumConstraints) = constraint_values.at(i, 0);
                    // 人為変数の係数を追加
                    if (constraint_values.at(i, 0) < 0)
                    {
                        auxiliary_tableau.at(i + 1, NumVariables + i) = -1;
                    }else
                    {
                        auxiliary_tableau.at(i + 1, NumVariables + i) = 1;
                    }
                }
                // インデックス
                // 0 ~ NumVariables-1まで非基底変数のインデックス
                // NumVariables ~ NumVariables + NumConstraints - 1まで基底変数のインデックス
                std::array<std::size_t, NumVariables + NumConstraints> auxiliary_variable_indices;
                // 人為変数を基底変数に指定
                // それ以外を非基底変数に指定
                for (std::size_t i = 0; i < NumVariables + NumConstraints; ++i)
                {
                    auxiliary_variable_indices.at(i) = i;
                }

                // 基底変数の相対コスト係数の列を0にする
                for (std::size_t i = 0; i < NumConstraints; ++i)
                {
                    auxiliary_tableau.add_row(i + 1, 0, -auxiliary_tableau.at(i + 1, NumVariables + i));
                }
                
                // 補助問題を解く
                solve_tableau(auxiliary_tableau, auxiliary_variable_indices);
                
                // debug
                std::cout << "auxiliary_tableau" << std::endl;
                for (std::size_t i = 0; i < NumConstraints + 1; ++i)
                {
                    for (std::size_t j = 0; j < NumVariables + NumConstraints + 1; ++j)
                    {
                        std::cout << auxiliary_tableau.at(i, j) << " ";
                    }
                    std::cout << std::endl;
                }
                
                // 補助問題の最小値が0でなければ、元の問題は実行不可能
                if ((int64_t)(auxiliary_tableau.at(0, NumVariables + NumConstraints) * 1e8) / (ValueType)(1e8) != 0)
                {
                    return std::unexpected(LPNoSolutionReason::Infeasible);
                }

                std::array<std::size_t, NumConstraints> indices_for_exclude;
                for (std::size_t i = 0; i < NumConstraints; ++i)
                {
                    indices_for_exclude.at(i) = NumVariables + i;
                }
                auto tableau = create_submatrix_excluding(auxiliary_tableau, {}, indices_for_exclude);
                std::cout << "test" << std::endl;
                // 目的関数の係数をコピー
                for (std::size_t i = 0; i < NumVariables; ++i)
                {
                    tableau.at(0, i) = objective_function_coefficients.at(i, 0);
                }
                std::cout << "test2" << std::endl;
                std::array<std::size_t, NumVariables> variable_indices;
                for (std::size_t i = 0; i < NumConstraints; ++i)
                {
                    variable_indices[NumVariables - NumConstraints + i] = auxiliary_variable_indices[NumVariables + i];
                }
                std::cout << "test3" << std::endl;
                for (std::size_t i = 0, j = 0; i < NumVariables; ++i)
                {
                    if (auxiliary_variable_indices[i] < NumVariables)
                    {
                        variable_indices[j] = auxiliary_variable_indices[i];
                        ++j;
                    }
                }
                std::cout << "test4" << std::endl;
                std::cout << "variable_indices" << std::endl;
                for (std::size_t i = 0; i < NumVariables; ++i)
                {
                    std::cout << variable_indices[i] << " ";
                }
                std::cout << std::endl;
                
                // 基底変数の相対コスト係数の列を0にする
                for (std::size_t i = 0; i < NumConstraints; ++i)
                {
                    tableau.add_row(i + 1, 0, -tableau.at(0, variable_indices[NumVariables - NumConstraints + i]));
                }
                std::cout << "test5" << std::endl;
                
                // 元の問題を解く
                solve_tableau(tableau, variable_indices);
                std::cout << "test6" << std::endl;
                Matrix<NumVariables, 1, ValueType> result;
                for (std::size_t i = 0; i < NumConstraints; ++i)
                {
                    result.at(variable_indices[NumVariables - NumConstraints + i], 0) = tableau.at(i + 1, NumVariables);
                }
                std::cout << "test7" << std::endl;
                
                return result;
            }
            
            private:
            template <std::size_t Rows, std::size_t Cols, typename ValueType>
            static constexpr bool is_optimal(const Matrix<Rows, Cols, ValueType>& tableau)
            {
                for (std::size_t i = 0; i < Cols - 1; ++i)
                {
                    if ((int)(tableau.at(0, i) * 1e8) / (ValueType)(1e8) < 0)
                    {
                        return false;
                    }
                }
                return true;
            }
            
            template <std::size_t Rows, std::size_t Cols, typename ValueType>
            static constexpr void solve_tableau(Matrix<Rows, Cols, ValueType>& tableau, std::array<std::size_t, Cols - 1>& indices)
            {
                constexpr std::size_t NumConstraints = Rows - 1;
                constexpr std::size_t NumVariables = Cols - 1;
                constexpr std::size_t num_base_variables = NumConstraints;
                constexpr std::size_t num_non_base_variables = NumVariables - num_base_variables;
                while (!is_optimal(tableau))
                {
                    std::cout << "tableau" << std::endl;
                    for (std::size_t i = 0; i < Rows; ++i)
                    {
                        for (std::size_t j = 0; j < Cols; ++j)
                        {
                            std::cout << tableau.at(i, j) << " ";
                        }
                        std::cout << std::endl;
                    }

                    // 交換する非基底変数を選ぶ
                    std::size_t x_k_index = 0;
                    ValueType min = 0;
                    for (std::size_t i = 0; i < num_non_base_variables; ++i)
                    {
                        auto cost = tableau.at(0, indices[i]);
                        if (cost < min)
                        {
                            min = cost;
                            x_k_index = i;
                        }
                    }
                    
                    // 交換する基底変数を選ぶ
                    std::size_t i_index = 0;
                    ValueType theta = std::numeric_limits<ValueType>::max();
                    for (std::size_t i = 0; i < NumConstraints; ++i)
                    {
                        auto frac_b_y = tableau.at(i + 1, NumVariables) / tableau.at(i + 1, indices[x_k_index]);
                        if (frac_b_y > 0 && theta > frac_b_y)
                        {
                            theta = frac_b_y;
                            i_index = i;
                        }
                    }
                    
                    // ピボット行を割る
                    tableau.multiply_row(i_index + 1, 1 / tableau.at(i_index + 1, indices[x_k_index]));
                    // 基底変数と非基底変数を交換する
                    for(std::size_t i = 0; i < Rows; ++i)
                    {
                        if (i == i_index + 1) continue;

                        tableau.add_row(i_index + 1, i, -tableau.at(i, indices[x_k_index]));
                    }
                    std::cout << "x_k_index: " << x_k_index << std::endl;
                    std::cout << "i_index: " << i_index << std::endl;
                    std::swap(indices[num_non_base_variables + i_index], indices[x_k_index]);
                    std::cout << "indices" << std::endl;
                    for (std::size_t i = 0; i < Cols - 1; ++i)
                    {
                        std::cout << indices[i] << " ";
                    }
                    std::cin.get();
                }
            }
        };
    }
}