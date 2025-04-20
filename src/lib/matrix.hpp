#pragma once
#include <cstdint>
#include <array>

#include "exception_helpter.hpp"

namespace mpi
{

    template <std::size_t Rows, std::size_t Cols, typename ValueType>
    class Matrix;

    /**
     * @brief 単位行列を生成する関数
     * @tparam Order 行列のサイズ
     * @tparam ValueType 行列の要素の型
     * @return 単位行列
     */
    template <std::size_t Order, typename ValueType = double>
        requires (Order > 0)
    constexpr Matrix<Order, Order, ValueType> IdentityMatrix() noexcept
    {
        Matrix<Order, Order, ValueType> result;
        for (std::size_t i = 0; i < Order; ++i)
        {
            result.at(i, i) = 1;
        }
        return result;
    }

    /**
     * @brief 行列積を計算する演算子オーバーロード
     * @tparam LhsRows 行列Aの行数
     * @tparam LhsCols_RhsRows 行列Aの列数、行列Bの行数
     * @tparam RhsCols 行列Bの列数
     * @tparam ValueType 行列の要素の型
     * @param lhs 行列A
     * @param rhs 行列B
     * @return 行列Aと行列Bの積
     */
    template <std::size_t LhsRows, std::size_t LhsCols_RhsRows, std::size_t RhsCols, typename ValueType>
    Matrix<LhsRows, RhsCols, ValueType> operator*(const Matrix<LhsRows, LhsCols_RhsRows, ValueType> &lhs, const Matrix<LhsCols_RhsRows, RhsCols, ValueType> &rhs) noexcept
    {
        Matrix<LhsRows, RhsCols, ValueType> result;
        for (std::size_t i = 0; i < LhsRows; ++i)
        {
            for (std::size_t j = 0; j < RhsCols; ++j)
            {
                result.data[i][j] = 0;
                for (std::size_t k = 0; k < LhsCols_RhsRows; ++k)
                {
                    result.data[i][j] += lhs.data[i][k] * rhs.data[k][j];
                }
            }
        }
        return result;
    }

    /**
     * @brief 行列の加算を計算する演算子オーバーロード
     * @tparam Rows 行列の行数
     * @tparam Cols 行列の列数
     * @tparam ValueType 行列の要素の型
     * @param lhs 行列A
     * @param rhs 行列B
     * @return 行列Aと行列Bの和
     */
    template <std::size_t Rows, std::size_t Cols, typename ValueType>
    Matrix<Rows, Cols, ValueType> operator+(const Matrix<Rows, Cols, ValueType> &lhs, const Matrix<Rows, Cols, ValueType> &rhs) noexcept
    {
        Matrix<Rows, Cols, ValueType> result;
        for (std::size_t i = 0; i < Rows; ++i)
        {
            for (std::size_t j = 0; j < Cols; ++j)
            {
                result.data[i][j] = lhs.data[i][j] + rhs.data[i][j];
            }
        }
        return result;
    }

    /**
     * @brief 行列の減算を計算する演算子オーバーロード
     * @tparam Rows 行列の行数
     * @tparam Cols 行列の列数
     * @tparam ValueType 行列の要素の型
     * @param lhs 行列A
     * @param rhs 行列B
     * @return 行列Aと行列Bの差
     */
    template <std::size_t Rows, std::size_t Cols, typename ValueType>
    Matrix<Rows, Cols, ValueType> operator-(const Matrix<Rows, Cols, ValueType> &lhs, const Matrix<Rows, Cols, ValueType> &rhs) noexcept
    {
        Matrix<Rows, Cols, ValueType> result;
        for (std::size_t i = 0; i < Rows; ++i)
        {
            for (std::size_t j = 0; j < Cols; ++j)
            {
                result.data[i][j] = lhs.data[i][j] - rhs.data[i][j];
            }
        }
        return result;
    }

    /**
     * @brief 行列とスカラーの積を計算する演算子オーバーロード
     * @tparam Rows 行列の行数
     * @tparam Cols 行列の列数
     * @tparam ValueType 行列の要素の型
     * @param scalar スカラー値
     * @param matrix 行列
     * @return スカラーと行列の積
     */
    template <std::size_t Rows, std::size_t Cols, typename ValueType>
    Matrix<Rows, Cols, ValueType> operator*(ValueType scalar, const Matrix<Rows, Cols, ValueType> &matrix) noexcept
    {
        Matrix<Rows, Cols, ValueType> result;
        for (std::size_t i = 0; i < Rows; ++i)
        {
            for (std::size_t j = 0; j < Cols; ++j)
            {
                result.data[i][j] = scalar * matrix.data[i][j];
            }
        }
        return result;
    }

    /**
     * @brief 行列クラス
     * @tparam Rows 行列の行数
     * @tparam Cols 行列の列数
     * @tparam ValueType 行列の要素の型
     */
    template <std::size_t Rows, std::size_t Cols, typename ValueType = double>
    class Matrix
    {
    private:
        static_assert(Rows > 0 && Cols > 0, "Rows and Cols must be greater than 0");
        
        static constexpr bool matrix_is_1x1 = (Rows == 1 && Cols == 1);
        static constexpr bool matrix_is_square = (Rows == Cols);

    public:
        constexpr Matrix(ValueType value) noexcept requires matrix_is_1x1
        : data{{{value}}} {}

        constexpr ValueType &at(std::size_t row, std::size_t col) &
        {
            if (row >= Rows || col >= Cols)
                exception::throw_exception<std::out_of_range>("Index out of range");

            return data[row][col];
        }
        constexpr const ValueType &at(std::size_t row, std::size_t col) const &
        {
            if (row >= Rows || col >= Cols)
                exception::throw_exception<std::out_of_range>("Index out of range");
            
            return data[row][col];
        }
        constexpr ValueType at(std::size_t row, std::size_t col) const &&
        {
            if (row >= Rows || col >= Cols)
                exception::throw_exception<std::out_of_range>("Index out of range");
            
            return data[row][col];
        }
        constexpr operator ValueType() const noexcept
            requires matrix_is_1x1
        {
            return data[0][0];
        }

        template <std::size_t LhsRows, std::size_t LhsCols_RhsRows, std::size_t RhsCols, typename ValueType_>
        friend Matrix<LhsRows, RhsCols, ValueType_> operator*(const Matrix<LhsRows, LhsCols_RhsRows, ValueType_> &lhs, const Matrix<LhsCols_RhsRows, RhsCols, ValueType_> &rhs) noexcept;
        template <std::size_t Rows_, std::size_t Cols_, typename ValueType_>
        friend Matrix<Rows_, Cols_, ValueType_> operator+(const Matrix<Rows_, Cols_, ValueType_> &lhs, const Matrix<Rows_, Cols_, ValueType_> &rhs) noexcept;
        template <std::size_t Rows_, std::size_t Cols_, typename ValueType_>
        friend Matrix<Rows_, Cols_, ValueType_> operator-(const Matrix<Rows_, Cols_, ValueType_> &lhs, const Matrix<Rows_, Cols_, ValueType_> &rhs) noexcept;
        template <std::size_t Rows_, std::size_t Cols_, typename ValueType_>
        friend Matrix<Rows_, Cols_, ValueType_> operator*(ValueType_ scalar, const Matrix<Rows_, Cols_, ValueType_> &matrix) noexcept;

    private:
        std::array<std::array<ValueType, Cols>, Rows> data = {};
    };
}