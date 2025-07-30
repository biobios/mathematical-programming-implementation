#pragma once

#include <ranges>
#include <type_traits>

#include "matrix.hpp"
#include "ranges_fixed_size.hpp"

namespace mpi
{

    /**
     * @brief 指定した行と列を除外した部分行列を作成する
     * @tparam Rows 元の行列の行数
     * @tparam Cols 元の行列の列数
     * @tparam ValueType 元の行列の要素の型
     * @tparam RemoveR 除外する行のインデックスの範囲の型
     * @tparam RemoveC 除外する列のインデックスの範囲の型
     * @param matrix 元の行列
     * @param remove_rows 除外する行のインデックスの範囲
     * @param remove_cols 除外する列のインデックスの範囲
     * @return 除外した部分行列
     */
    template <std::size_t Rows, std::size_t Cols, typename ValueType, mpi::ranges::fixed_size_range RemoveR = std::array<std::size_t, 0>, mpi::ranges::fixed_size_range RemoveC = std::array<std::size_t, 0>>
        requires std::integral<std::ranges::range_value_t<RemoveR>> && std::integral<std::ranges::range_value_t<RemoveC>>
    Matrix<Rows - mpi::ranges::fixed_size_range_size<RemoveR>, Cols - mpi::ranges::fixed_size_range_size<RemoveC>, ValueType> create_submatrix_excluding(
        const Matrix<Rows, Cols, ValueType> &matrix, RemoveR&& remove_rows, RemoveC&& remove_cols = {}) noexcept
    {
        constexpr std::size_t ExRows = mpi::ranges::fixed_size_range_size<RemoveR>;
        constexpr std::size_t ExCols = mpi::ranges::fixed_size_range_size<RemoveC>;
        Matrix<Rows - ExRows, Cols - ExCols, ValueType> result;
        for(std::size_t i = 0, r = 0; i < Rows; ++i)
        {
            if (std::ranges::find(remove_rows, i) == std::ranges::end(remove_rows))
            {
                for (std::size_t j = 0, c = 0; j < Cols; ++j)
                {
                    if (std::ranges::find(remove_cols, j) == std::ranges::end(remove_cols))
                    {
                        result.at(r, c) = matrix.at(i, j);
                        ++c;
                    }
                }
                ++r;
            }
        }
        return result;
    }

    /**
     * @brief 指定した行と列を除外した部分行列を作成する
     * @tparam Rows 元の行列の行数
     * @tparam Cols 元の行列の列数
     * @tparam ExRows 除外する行の数
     * @tparam ExCols 除外する列の数
     * @tparam ValueType 元の行列の要素の型
     * @tparam IndexType1 除外する行のインデックスの型
     * @tparam IndexType2 除外する列のインデックスの型
     * @param matrix 元の行列
     * @param remove_rows 除外する行のインデックスの配列
     * @param remove_cols 除外する列のインデックスの配列
     * @return 除外した部分行列
     * @note この関数はcreate_submatrix_excluding(matrix, {1, 2}, {3, 4})のように、波かっこで囲まれたインデックスにマッチします。
     */
    template <std::size_t Rows, std::size_t Cols, std::size_t ExRows, std::size_t ExCols, typename ValueType, std::integral IndexType1, std::integral IndexType2>
    Matrix<Rows - ExRows, Cols - ExCols, ValueType> create_submatrix_excluding(
        const Matrix<Rows, Cols, ValueType> &matrix, const IndexType1 (&remove_rows)[ExRows], const IndexType2 (&remove_cols)[ExCols]) noexcept
    {
        return create_submatrix_excluding<Rows, Cols, ValueType>(matrix, remove_rows, remove_cols);
    }

    /**
     * @brief 指定した行を除外した部分行列を作成する
     * @tparam Rows 元の行列の行数
     * @tparam Cols 元の行列の列数
     * @tparam ExRows 除外する行の数
     * @tparam ValueType 元の行列の要素の型
     * @tparam IndexType 除外する行のインデックスの型
     * @param matrix 元の行列
     * @param remove_rows 除外する行のインデックスの配列
     * @return 除外した部分行列
     * @note
     * この関数はcreate_submatrix_excluding(matrix, {1, 2}, {})のように、
     * 第２引数が波かっこで囲まれたインデックスであり、
     * 第３引数が空の波かっこ、あるいは省略された場合にマッチします。
     */
    template <std::size_t Rows, std::size_t Cols, std::size_t ExRows, typename ValueType, std::integral IndexType>
    Matrix<Rows - ExRows, Cols, ValueType> create_submatrix_excluding(
        const Matrix<Rows, Cols, ValueType> &matrix, const IndexType (&remove_rows)[ExRows], const std::array<std::size_t, 0> = {}) noexcept
    {
        return create_submatrix_excluding<Rows, Cols, ValueType>(matrix, remove_rows, {});
    }

    /**
     * @brief 指定した列を除外した部分行列を作成する
     * @tparam Rows 元の行列の行数
     * @tparam Cols 元の行列の列数
     * @tparam ExCols 除外する列の数
     * @tparam ValueType 元の行列の要素の型
     * @tparam IndexType 除外する列のインデックスの型
     * @param matrix 元の行列
     * @param remove_cols 除外する列のインデックスの配列
     * @return 除外した部分行列
     * @note
     * この関数はcreate_submatrix_excluding(matrix, {}, {3, 4})のように、
     * 第２引数が空の波かっこであり、第３引数が波かっこで囲まれたインデックスであるときにマッチします。
     */
    template <std::size_t Rows, std::size_t Cols, std::size_t ExCols, typename ValueType, std::integral IndexType>
    Matrix<Rows, Cols - ExCols, ValueType> create_submatrix_excluding(
        const Matrix<Rows, Cols, ValueType> &matrix, const std::array<std::size_t, 0>, const IndexType (&remove_cols)[ExCols]) noexcept
    {
        return create_submatrix_excluding<Rows, Cols, ValueType>(matrix, {}, remove_cols);
    }

    /**
     * @brief 指定した行と列を選択した部分行列を作成する
     * @tparam Rows 元の行列の行数
     * @tparam Cols 元の行列の列数
     * @tparam ValueType 元の行列の要素の型
     * @tparam SelectR 選択する行のインデックスの範囲の型
     * @tparam SelectC 選択する列のインデックスの範囲の型
     * @param matrix 元の行列
     * @param selected_rows 選択する行のインデックスの範囲
     * @param selected_cols 選択する列のインデックスの範囲
     * @return 選択した部分行列
     * @note 長さが0の範囲を指定した場合、すべての行または列が選択されます。
     */
    template <std::size_t Rows, std::size_t Cols, typename ValueType, mpi::ranges::fixed_size_range SelectR = std::array<std::size_t, 0>, mpi::ranges::fixed_size_range SelectC = std::array<std::size_t, 0>>
        requires std::integral<std::ranges::range_value_t<SelectR>> && std::integral<std::ranges::range_value_t<SelectC>>
    Matrix<mpi::ranges::fixed_size_range_size<SelectR> == 0 ? Rows : mpi::ranges::fixed_size_range_size<SelectR>, mpi::ranges::fixed_size_range_size<SelectC> == 0 ? Cols : mpi::ranges::fixed_size_range_size<SelectC>, ValueType> create_submatrix_selecting(
        const Matrix<Rows, Cols, ValueType> &matrix, SelectR&& selected_rows, SelectC&& selected_cols = {}) noexcept
    {
        constexpr std::size_t SelectedRows = mpi::ranges::fixed_size_range_size<SelectR> == 0 ? Rows : mpi::ranges::fixed_size_range_size<SelectR>;
        constexpr std::size_t SelectedCols = mpi::ranges::fixed_size_range_size<SelectC> == 0 ? Cols : mpi::ranges::fixed_size_range_size<SelectC>;
        Matrix<SelectedRows, SelectedCols, ValueType> result;

        if constexpr(mpi::ranges::fixed_size_range_size<SelectR> == 0 && mpi::ranges::fixed_size_range_size<SelectC> == 0)
        {
            // コピーコンストラクタを定義したら置き換える
            for(std::size_t r = 0; r < Rows; ++r)
            {
                for(std::size_t c = 0; c < Cols; ++c)
                {
                    result.at(r, c) = matrix.at(r, c);
                }
            }
        }
        else if(mpi::ranges::fixed_size_range_size<SelectR> == 0)
        {
            for(std::size_t r = 0; r < Rows; ++r)
            {
                for(std::size_t c = 0; auto col : selected_cols)
                {
                    result.at(r, c) = matrix.at(r, col);
                    ++c;
                }
            }
        }
        else if(mpi::ranges::fixed_size_range_size<SelectC> == 0)
        {
            for(std::size_t r = 0; auto row : selected_rows)
            {
                for(std::size_t c = 0; c < Cols; ++c)
                {
                    result.at(r, c) = matrix.at(row, c);
                }
                ++r;
            }
        }
        else
        {
            for(std::size_t r = 0; auto row : selected_rows)
            {
                for(std::size_t c = 0; auto col : selected_cols)
                {
                    result.at(r, c) = matrix.at(row, col);
                    ++c;
                }
                ++r;
            }
        }

        return result;
    }

    /**
     * @brief 指定した行と列を選択した部分行列を作成する
     * @tparam Rows 元の行列の行数
     * @tparam Cols 元の行列の列数
     * @tparam SelectedRows 選択する行の数
     * @tparam SelectedCols 選択する列の数
     * @tparam ValueType 元の行列の要素の型
     * @tparam IndexType1 選択する行のインデックスの型
     * @tparam IndexType2 選択する列のインデックスの型
     * @param matrix 元の行列
     * @param select_rows 選択する行のインデックスの配列
     * @param select_cols 選択する列のインデックスの配列
     * @return 選択した部分行列
     * @note この関数はcreate_submatrix_selecting(matrix, {1, 2}, {3, 4})のように、波かっこで囲まれたインデックスにマッチします。
     */
    template <std::size_t Rows, std::size_t Cols, std::size_t SelectedRows, std::size_t SelectedCols, typename ValueType, std::integral IndexType1, std::integral IndexType2>
    Matrix<SelectedRows == 0 ? Rows : SelectedRows, SelectedCols == 0 ? Cols : SelectedCols, ValueType> create_submatrix_selecting(
        const Matrix<Rows, Cols, ValueType> &matrix, const IndexType1 (&select_rows)[SelectedRows], const IndexType2 (&select_cols)[SelectedCols] = {}) noexcept
    {
        return create_submatrix_selecting<Rows, Cols, ValueType>(matrix, select_rows, select_cols);
    }
    
    /**
     * @brief 指定した行を選択した部分行列を作成する
     * @tparam Rows 元の行列の行数
     * @tparam Cols 元の行列の列数
     * @tparam SelectedRows 選択する行の数
     * @tparam ValueType 元の行列の要素の型
     * @tparam IndexType 選択する行のインデックスの型
     * @param matrix 元の行列
     * @param select_rows 選択する行のインデックスの配列
     * @return 選択した部分行列
     * @note この関数はcreate_submatrix_selecting(matrix, {1, 2}, {})のように、
     * 第２引数が波かっこで囲まれたインデックスであり、
     * 第３引数が空の波かっこ、あるいは省略された場合にマッチします。
     */
    template <std::size_t Rows, std::size_t Cols, std::size_t SelectedRows, typename ValueType, std::integral IndexType>
    Matrix<SelectedRows, Cols, ValueType> create_submatrix_selecting(
        const Matrix<Rows, Cols, ValueType> &matrix, const IndexType (&select_rows)[SelectedRows], const std::array<std::size_t, 0> = {}) noexcept
    {
        return create_submatrix_selecting<Rows, Cols, ValueType>(matrix, select_rows);
    }

    /**
     * @brief 指定した列を選択した部分行列を作成する
     * @tparam Rows 元の行列の行数
     * @tparam Cols 元の行列の列数
     * @tparam SelectedCols 選択する列の数
     * @tparam ValueType 元の行列の要素の型
     * @tparam IndexType 選択する列のインデックスの型
     * @param matrix 元の行列
     * @param select_cols 選択する列のインデックスの配列
     * @return 選択した部分行列
     * @note この関数はcreate_submatrix_selecting(matrix, {}, {3, 4})のように、
     * 第２引数が空の波かっこであり、第３引数が波かっこで囲まれたインデックスであるときにマッチします。
     */
    template <std::size_t Rows, std::size_t Cols, std::size_t SelectedCols, typename ValueType, std::integral IndexType>
    Matrix<Rows, SelectedCols, ValueType> create_submatrix_selecting(
        const Matrix<Rows, Cols, ValueType> &matrix, const std::array<std::size_t, 0>, const IndexType (&select_cols)[SelectedCols]) noexcept
    {
        return create_submatrix_selecting<Rows, Cols, ValueType>(matrix, select_cols);
    }
}