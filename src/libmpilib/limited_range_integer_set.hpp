#pragma once

#include <cstddef>
#include <vector>
#include <limits>
#include <stdexcept>

namespace mpi
{
    /**
     * @brief 制限付き範囲整数集合を表すクラス
     */
    class LimitedRangeIntegerSet {
    public:
        using size_type = size_t;
        using iterator = typename std::vector<size_t>::const_iterator;
        using const_iterator = typename std::vector<size_t>::const_iterator;
        enum class InitSet {
            /**
             * @brief 空集合で初期化する
             */
            Empty,
            /**
             * @brief 全ての整数を含む集合で初期化する
             */
            Universal
        };

        /**
         * @brief 指定した範囲と初期化セットで集合を構築する
         * @param max 集合に含まれる最大の整数値
         * @param init_set 初期化セット
         * @param min 集合に含まれる最小の整数値 (デフォルトは0)
         */
        LimitedRangeIntegerSet(size_t max, InitSet init_set = InitSet::Universal, size_t min = 0) 
            : MAX(max), MIN(min)
        {
            switch (init_set) {
                case InitSet::Empty:
                    positions.resize(max - min + 1, std::numeric_limits<size_t>::max());
                    break;
                case InitSet::Universal:
                    elements.reserve(max - min + 1);
                    positions.reserve(max - min + 1);
                    for (size_t i = 0; i <= max - min; ++i) {
                        elements.push_back(i + min);
                        positions.push_back(i);
                    }
                    break;
                default:
                    throw std::invalid_argument("Invalid initialization set");
            }
        }
        
        /**
         * @brief 集合を指定した初期化セットでリセットする
         * @param init_set 初期化セット
         */
        void reset(InitSet init_set = InitSet::Universal)
        {
            switch (init_set) {
                case InitSet::Empty:
                    elements.clear();
                    positions.assign(MAX - MIN + 1, std::numeric_limits<size_t>::max());
                    break;
                case InitSet::Universal:
                    elements.clear();
                    elements.reserve(MAX - MIN + 1);
                    positions.clear();
                    for (size_t i = 0; i <= MAX - MIN; ++i) {
                        elements.push_back(i + MIN);
                        positions.push_back(i);
                    }
                    break;
                default:
                    throw std::invalid_argument("Invalid initialization set");
            }
        }
        
        /**
         * @brief 指定した値が集合に含まれているかを判定する
         * @param value 判定する値
         * @return 含まれている場合はtrue、そうでない場合はfalse
         */
        bool contains(size_t value) const
        {
            if (value < MIN || value > MAX) {
                return false;
            }
            
            return positions[value - MIN] != std::numeric_limits<size_t>::max();
        }
        
        /**
         * @brief 指定した値を集合から削除する
         * @param value 削除する値
         * @return 削除に成功した場合は1、値が存在しない場合は0
         */
        size_type erase(size_t value)
        {
            if (value < MIN || value > MAX) {
                return 0;
            }
            
            size_t pos = positions[value - MIN];
            if (pos == std::numeric_limits<size_t>::max()) {
                return 0; // 値が存在しない
            }
            
            size_t last_value = elements.back();
            elements[pos] = last_value;
            positions[last_value - MIN] = pos;
            elements.pop_back();
            positions[value - MIN] = std::numeric_limits<size_t>::max();
            
            return 1; // 成功
        }
        
        /**
         * @brief 指定した値を集合に挿入する
         * @param value 挿入する値
         * @return 挿入された要素へのイテレータと、挿入が新たに行われたかどうかのペア
         */
        std::pair<iterator, bool> insert(size_t value)
        {
            if (value < MIN || value > MAX) {
                throw std::out_of_range("Value out of range");
            }
            
            size_t pos = positions[value - MIN];
            if (pos != std::numeric_limits<size_t>::max()) {
                return {elements.begin() + pos, false}; // 既に存在する
            }
            
            elements.push_back(value);
            pos = elements.size() - 1;
            positions[value - MIN] = pos;
            
            return {elements.begin() + pos, true}; // 新たに挿入された
        }
        
        /**
         * @brief 集合の要素へのイテレータの始まりを返す
         * @return イテレータの始まり
         */
        iterator begin() const
        {
            return elements.cbegin();
        }
        
        /**
         * @brief 集合の要素へのイテレータの終わりを返す
         * @return イテレータの終わり
         */
        const_iterator cbegin() const
        {
            return elements.cbegin();
        }

        /**
         * @brief 集合の要素へのイテレータの終わりを返す
         * @return イテレータの終わり
         */
        iterator end() const
        {
            return elements.cend();
        }
        
        /**
         * @brief 集合の要素へのイテレータの終わりを返す
         * @return イテレータの終わり
         */
        const_iterator cend() const
        {
            return elements.cend();
        }
        
        /**
         * @brief 集合の要素数を返す
         * @return 要素数
         */
        size_type size() const
        {
            return elements.size();
        }
        
    private:
        const size_t MAX;
        const size_t MIN;
        std::vector<size_t> elements;
        std::vector<size_t> positions;
    };
}