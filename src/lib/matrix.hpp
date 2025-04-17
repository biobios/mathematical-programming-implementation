#pragma once
#include <cstdint>
#include <array>

namespace mpi
{

    template <std::size_t Rows, std::size_t Cols, typename ValueType>
    class Matrix;

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
    Matrix<LhsRows, RhsCols, ValueType> operator*(const Matrix<LhsRows, LhsCols_RhsRows, ValueType> &lhs, const Matrix<LhsCols_RhsRows, RhsCols, ValueType> &rhs)
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
    Matrix<Rows, Cols, ValueType> operator+(const Matrix<Rows, Cols, ValueType> &lhs, const Matrix<Rows, Cols, ValueType> &rhs)
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
    Matrix<Rows, Cols, ValueType> operator-(const Matrix<Rows, Cols, ValueType> &lhs, const Matrix<Rows, Cols, ValueType> &rhs)
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
    Matrix<Rows, Cols, ValueType> operator*(ValueType scalar, const Matrix<Rows, Cols, ValueType> &matrix)
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
     * @brief 行列の基本クラス
     * @tparam Rows 行列の行数
     * @tparam Cols 行列の列数
     * @tparam ValueType 行列の要素の型
     */
    template <std::size_t Rows, std::size_t Cols, typename ValueType>
    class MatrixBase
    {
    public:
        constexpr ValueType &at(std::size_t row, std::size_t col) &
        {
            return data[row][col];
        }
        constexpr const ValueType &at(std::size_t row, std::size_t col) const &
        {
            return data[row][col];
        }
        constexpr ValueType at(std::size_t row, std::size_t col) const &&
        {
            return data[row][col];
        }

        template <std::size_t LhsRows, std::size_t LhsCols_RhsRows, std::size_t RhsCols, typename ValueType_>
        friend Matrix<LhsRows, RhsCols, ValueType_> operator*(const Matrix<LhsRows, LhsCols_RhsRows, ValueType_> &lhs, const Matrix<LhsCols_RhsRows, RhsCols, ValueType_> &rhs);
        template <std::size_t Rows_, std::size_t Cols_, typename ValueType_>
        friend Matrix<Rows_, Cols_, ValueType_> operator+(const Matrix<Rows_, Cols_, ValueType_> &lhs, const Matrix<Rows_, Cols_, ValueType_> &rhs);
        template <std::size_t Rows_, std::size_t Cols_, typename ValueType_>
        friend Matrix<Rows_, Cols_, ValueType_> operator-(const Matrix<Rows_, Cols_, ValueType_> &lhs, const Matrix<Rows_, Cols_, ValueType_> &rhs);
        template <std::size_t Rows_, std::size_t Cols_, typename ValueType_>
        friend Matrix<Rows_, Cols_, ValueType_> operator*(ValueType_ scalar, const Matrix<Rows_, Cols_, ValueType_> &matrix);

    private:
        std::array<std::array<ValueType, Cols>, Rows> data;
    };

    /**
     * @brief 行列クラス
     * @tparam Rows 行列の行数
     * @tparam Cols 行列の列数
     * @tparam ValueType 行列の要素の型
     */
    template <std::size_t Rows, std::size_t Cols, typename ValueType = double>
    class Matrix : public MatrixBase<Rows, Cols, ValueType>
    {
    public:
    private:
    };

    /**
     * @brief 1x1行列クラス
     * @tparam ValueType 行列の要素の型
     */
    template <typename ValueType>
    class Matrix<1, 1, ValueType> : public MatrixBase<1, 1, ValueType>
    {
    public:
        constexpr Matrix() = default;
        constexpr Matrix(ValueType value)
        {
            this->at(0, 0) = value;
        }

        constexpr operator ValueType() const
        {
            return this->at(0, 0);
        }

    private:
    };
}