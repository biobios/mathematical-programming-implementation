#pragma once
#include <cstdint>
#include <array>

#include "exception_helper.hpp"

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
        
        /**
         * @brief デフォルトコンストラクタ
         * @note 行列の要素はデフォルト値で初期化される
         */
        constexpr Matrix() noexcept : data{} {}

        /**
         * @brief 初期化リストを使用して行列を初期化するコンストラクタ
         * @param init_data 初期化リスト
         * @note 初期化子リストのサイズが行列のサイズと一致しない場合、足りない部分はデフォルト値で初期化され、はみ出た部分は無視される
         */
        constexpr Matrix(std::initializer_list<std::initializer_list<ValueType>> init_data) noexcept
        {
            auto row_it = init_data.begin();
            auto row_end = init_data.end();
            for (std::size_t i = 0; row_it != row_end && i < Rows; ++i, ++row_it)
            {
                auto elem_it = row_it->begin();
                auto elem_end = row_it->end();
                for (std::size_t j = 0; elem_it != elem_end && j < Cols; ++j, ++elem_it)
                {
                    data[i][j] = *elem_it;
                }
            }
        }

        /**
         * @brief 値から1x1行列に変換するコンストラクタ
         * @param value 値
         */
        constexpr Matrix(ValueType value) noexcept requires matrix_is_1x1
        : data{{{value}}} {}

        /**
         * @brief 行列の要素にアクセスするための関数
         * @param row 行インデックス
         * @param col 列インデックス
         * @return 行列の要素への参照
         * @exception std::out_of_range インデックスが範囲外の場合
         */
        constexpr ValueType &at(std::size_t row, std::size_t col) &
        {
            if (row >= Rows || col >= Cols)
                exception::throw_exception<std::out_of_range>("Index out of range : row = " + std::to_string(row) + ", col = " + std::to_string(col));

            return data[row][col];
        }

        /**
         * @brief 行列の要素にアクセスするための関数
         * @param row 行インデックス
         * @param col 列インデックス
         * @return 行列の要素へのconst参照
         * @exception std::out_of_range インデックスが範囲外の場合
         */
        constexpr const ValueType &at(std::size_t row, std::size_t col) const &
        {
            if (row >= Rows || col >= Cols)
                exception::throw_exception<std::out_of_range>("Index out of range");
            
            return data[row][col];
        }

        /**
         * @brief 行列の要素にアクセスするための関数
         * @param row 行インデックス
         * @param col 列インデックス
         * @return 行列の要素のコピー
         * @exception std::out_of_range インデックスが範囲外の場合
         */
        constexpr ValueType at(std::size_t row, std::size_t col) const &&
        {
            if (row >= Rows || col >= Cols)
                exception::throw_exception<std::out_of_range>("Index out of range");
            
            return data[row][col];
        }

        /**
         * @brief 行列の転置を計算する関数
         * @return 転置行列
         */
        constexpr Matrix<Cols, Rows, ValueType> transpose() const noexcept
        {
            Matrix<Cols, Rows, ValueType> result;
            for (std::size_t i = 0; i < Rows; ++i)
            {
                for (std::size_t j = 0; j < Cols; ++j)
                {
                    result.at(j, i) = data[i][j];
                }
            }
            return result;
        }

        /**
         * @brief 行列の逆行列を計算する関数
         * @return 逆行列
         * @exception std::domain_error 行列が正則でない場合
         */
        constexpr Matrix calc_inverse() const
            requires matrix_is_square
        {
            Matrix result = IdentityMatrix<Rows, ValueType>();
            Matrix copy = *this;

            for (std::size_t i = 0; i < Rows; ++i)
            {
                if (copy.at(i, i) == 0)
                {
                    bool found = false;

                    for (std::size_t j = i + 1; j < Rows; ++j)
                    {
                        if (copy.at(j, i) != 0)
                        {
                            result.switch_row(i, j);
                            copy.switch_row(i, j);
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                        exception::throw_exception<std::domain_error>("Matrix is singular");
                }

                ValueType pivot = copy.at(i, i);
                if (pivot != 1)
                {
                    result.multiply_row(i, 1 / pivot);
                    copy.multiply_row(i, 1 / pivot);
                }

                for (std::size_t j = 0; j < Rows; ++j)
                {
                    if (j != i)
                    {
                        ValueType factor = copy.at(j, i);
                        result.add_row(i, j, -factor);
                        copy.add_row(i, j, -factor);
                    }
                }                
            }

            return result;
        }

        /**
         * @brief 行列の行を入れ替える関数
         * @param row1 行インデックス1
         * @param row2 行インデックス2
         * @return 自身の参照
         * @exception std::out_of_range インデックスが範囲外の場合
         */
        constexpr Matrix& switch_row(std::size_t row1, std::size_t row2) &
        {
            if (row1 >= Rows || row2 >= Rows)
                exception::throw_exception<std::out_of_range>("Index out of range");

            std::swap(data[row1], data[row2]);

            return *this;
        }

        /**
         * @brief 行列の列を入れ替える関数
         * @param col1 列インデックス1
         * @param col2 列インデックス2
         * @return 自身の参照
         * @exception std::out_of_range インデックスが範囲外の場合
         */
        constexpr Matrix& switch_col(std::size_t col1, std::size_t col2) &
        {
            if (col1 >= Cols || col2 >= Cols)
                exception::throw_exception<std::out_of_range>("Index out of range");

            for (std::size_t i = 0; i < Rows; ++i)
            {
                std::swap(data[i][col1], data[i][col2]);
            }

            return *this;
        }

        /**
         * @brief 行列の行をスカラー倍する関数
         * @param row 行インデックス
         * @param factor スカラー値
         * @return 自身の参照
         * @exception std::out_of_range インデックスが範囲外の場合
         */
        constexpr Matrix& multiply_row(std::size_t row, ValueType factor) &
        {
            if (row >= Rows)
                exception::throw_exception<std::out_of_range>("Index out of range");

            for (std::size_t j = 0; j < Cols; ++j)
            {
                data[row][j] *= factor;
            }

            return *this;
        }

        /**
         * @brief 行列の列をスカラー倍する関数
         * @param col 列インデックス
         * @param factor スカラー値
         * @return 自身の参照
         * @exception std::out_of_range インデックスが範囲外の場合
         */
        constexpr Matrix& multiply_col(std::size_t col, ValueType factor) &
        {
            if (col >= Cols)
                exception::throw_exception<std::out_of_range>("Index out of range");

            for (std::size_t i = 0; i < Rows; ++i)
            {
                data[i][col] *= factor;
            }

            return *this;
        }

        /**
         * @brief 行列の行を別の行に加算する関数
         * @param source_row 加算元行インデックス
         * @param target_row 加算先行インデックス
         * @param factor この値が掛けられた値が加算される
         * @return 自身の参照
         * @exception std::out_of_range インデックスが範囲外の場合
         */
        constexpr Matrix& add_row(std::size_t source_row, std::size_t target_row, ValueType factor) &
        {
            if (source_row >= Rows || target_row >= Rows)
                exception::throw_exception<std::out_of_range>("Index out of range");

            for (std::size_t j = 0; j < Cols; ++j)
            {
                data[target_row][j] += factor * data[source_row][j];
            }

            return *this;
        }

        /**
         * @brief 行列の列を別の列に加算する関数
         * @param source_col 加算元列インデックス
         * @param target_col 加算先列インデックス
         * @param factor この値が掛けられた値が加算される
         * @return 自身の参照
         * @exception std::out_of_range インデックスが範囲外の場合
         */
        constexpr Matrix& add_col(std::size_t source_col, std::size_t target_col, ValueType factor) &
        {
            if (source_col >= Cols || target_col >= Cols)
                exception::throw_exception<std::out_of_range>("Index out of range");

            for (std::size_t i = 0; i < Rows; ++i)
            {
                data[i][target_col] += factor * data[i][source_col];
            }

            return *this;
        }

        /**
         * @brief 1x1行列を値に変換する演算子オーバーロード
         * @return 行列の要素
         */
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