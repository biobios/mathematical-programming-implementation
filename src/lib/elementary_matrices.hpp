#pragma once

#include <cstdint>
#include "exception_helpter.hpp"
#include "matrix.hpp"

namespace mpi
{

    namespace elementary_matrices
    {
        /**
         * @brief 行(or列)を入れ替える基本行列を生成する
         * @tparam Order 行列のサイズ
         * @tparam ValueType 行列の要素の型
         * @param index1 入れ替える行(or列)のインデックス
         * @param index2 入れ替える行(or列)のインデックス
         * @return 行(or列)を入れ替える基本行列
         * @exception std::out_of_range 行(or列)のインデックスが範囲外の場合
         * @note 左から掛けると行を入れ替え、右から掛けると列を入れ替える
         */
        template <std::size_t Order, typename ValueType = double>
        constexpr Matrix<Order, Order, ValueType> SwitchingMatrix(std::size_t index1, std::size_t index2)
        {
            if (index1 >= Order || index2 >= Order)
                exception::throw_exception<std::out_of_range>("Index out of range");

            Matrix<Order, Order, ValueType> result = IdentityMatrix<Order, ValueType>();

            result.at(index1, index1) = 0;
            result.at(index2, index2) = 0;
            result.at(index1, index2) = 1;
            result.at(index2, index1) = 1;
            
            return result;
        }

        /**
         * @brief 行(or列)をスカラー倍する基本行列を生成する
         * @tparam Order 行列のサイズ
         * @tparam ValueType 行列の要素の型
         * @param index スカラー倍する行(or列)のインデックス
         * @param factor スカラー倍する値
         * @return 行(or列)をスカラー倍する基本行列
         * @exception std::out_of_range 行(or列)のインデックスが範囲外の場合
         * @exception std::invalid_argument スカラー倍する値が0の場合
         * @note 左から掛けると行をスカラー倍し、右から掛けると列をスカラー倍する
         */
        template <std::size_t Order, typename ValueType = double>
        constexpr Matrix<Order, Order, ValueType> MultiplicationMatrix(std::size_t index, ValueType factor)
        {
            if (index >= Order)
                exception::throw_exception<std::out_of_range>("Index out of range");
            if (factor == 0)
                exception::throw_exception<std::invalid_argument>("Factor cannot be zero");
            
            Matrix<Order, Order, ValueType> result = IdentityMatrix<Order, ValueType>();

            result.at(index, index) = factor;
            return result;
        }

        /**
         * @brief 行(or列)の定数倍を別の行に加算する基本行列を生成する
         * @tparam Order 行列のサイズ
         * @tparam ValueType 行列の要素の型
         * @param index1 加算する行(or列)のインデックス
         * @param index2 加算される行(or列)のインデックス
         * @param factor 定数倍する値
         * @return 行(or列)の定数倍を別の行に加算する基本行列
         * @exception std::out_of_range 行(or列)のインデックスが範囲外の場合
         * @exception std::invalid_argument 行(or列)のインデックスが同じ場合
         * @note 左から掛けると行を加算し、右から掛けると列を加算する
         */
        template <std::size_t Order, typename ValueType = double>
        constexpr Matrix<Order, Order, ValueType> AdditionMatrix(std::size_t index1, std::size_t index2, ValueType factor)
        {
            if (index1 >= Order || index2 >= Order)
                exception::throw_exception<std::out_of_range>("Index out of range");
            if (index1 == index2)
                exception::throw_exception<std::invalid_argument>("Cannot add a row (or colmn) to itself");
            Matrix<Order, Order, ValueType> result = IdentityMatrix<Order, ValueType>();

            result.at(index1, index2) = factor;
            return result;
        }
    }
}