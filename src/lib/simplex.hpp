#pragma once

#include <expected>

#include "matrix.hpp"
#include "ranges_fixed_size.hpp"
#include "linear_programming.hpp"
#include "combination.hpp"
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
                // 基底変数の個数
                constexpr std::size_t num_base_variables = NumConstraints;
                // 非基底変数の個数
                constexpr std::size_t num_non_base_variables = NumVariables - num_base_variables;

                auto& b = constraint_values;

                // 0 ~ num_base_variables-1まで基底変数のインデックス
                // num_base_variables ~ num_variables-1まで非基底変数のインデックス
                std::array<std::size_t, NumVariables> indices;

                for (std::size_t i = 0; i < NumVariables; ++i)
                {
                    indices[i] = i;
                }

                // (0) 初期実行可能基底解を選ぶ

                bool found_init_feasible = false;

                do{
                    auto B = create_submatrix_selecting(constraints_coefficients, {}, ranges::fixed_size<num_base_variables>(0)(indices));
                    auto B_inv = B.calc_inverse();
                    auto b_bar = B_inv * b;
    
                    if(is_feasible_basis_solution(b_bar))
                    {
                        found_init_feasible = true;
                        break;
                    }
                }while(next_combination(indices, num_base_variables));

                if(!found_init_feasible)
                {
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
        };
    }
}