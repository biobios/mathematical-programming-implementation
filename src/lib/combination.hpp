#pragma once

#include <iterator>
#include <ranges>
#include <algorithm>
#include <functional>

namespace mpi
{
    /**
     * @brief 次の組み合わせを生成する関数
     * @tparam KindsIterator 組み合わせのイテレータの型
     * @tparam KindsSentinel 組み合わせの番兵の型
     * @tparam Comp 比較関数の型
     * @tparam Proj プロジェクタの型
     * @param first 組み合わせの範囲の先頭
     * @param last 組み合わせの範囲の末尾
     * @param k 組み合わせのサイズ
     * @param comp 比較関数
     * @param proj 射影関数
     * @return true 次の組み合わせが存在する場合
     * @return false 次の組み合わせが存在しない場合
     * @note 0 <= i < k の範囲、 k <= i < n の範囲でそれぞれソートされている必要があります。
     */
    template <std::bidirectional_iterator KindsIterator, std::sentinel_for<KindsIterator> KindsSentinel, typename Comp = std::ranges::less, typename Proj = std::identity>
        requires std::sortable<KindsIterator, Comp, Proj>
    constexpr bool next_combination(KindsIterator first, KindsSentinel last, std::size_t k, Comp comp = {}, Proj proj = {})
    {
        auto comb_end = std::ranges::next(first, k, last);

        if (comb_end == last)
            return false;

        auto current = comb_end;
        auto found = last;
        do{
            current = std::ranges::prev(current, 1, first);

            // if(*current >= *(last - 1))
            //if(!(*current < *(last - 1)))
            if(!std::invoke(comp, std::invoke(proj, *current), std::invoke(proj, *(last - 1))))
                continue;

            found = std::ranges::upper_bound(comb_end, last, *current, comp, proj);
        }while(current != first && found == last);

        if (found == last){
            std::ranges::rotate(first, comb_end, last);
            return false;
        }

        std::ranges::iter_swap(current, found);

        ++current;
        ++found;

        std::ranges::reverse(current, comb_end);
        std::ranges::reverse(found, last);

        auto back_current = last;

        while(current != comb_end && found != back_current)
        {
            std::ranges::iter_swap(current, (back_current - 1));
            ++current;
            --back_current;
        }

        while((back_current - found) > 0)
        {
            std::ranges::iter_swap((back_current - 1), found);
            ++found;
            --back_current;
        }

        while((comb_end - current) > 0)
        {
            std::ranges::iter_swap((comb_end - 1), current);
            ++current;
            --comb_end;
        }

        return true;
    }

    /**
     * @brief 次の組み合わせを生成する関数
     * @tparam Kinds 組み合わせの範囲の型
     * @tparam Comp 比較関数の型
     * @tparam Proj プロジェクタの型
     * @param kinds 組み合わせの範囲
     * @param k 組み合わせのサイズ
     * @param comp 比較関数
     * @param proj 射影関数
     * @return true 次の組み合わせが存在する場合
     * @return false 次の組み合わせが存在しない場合
     * @note 0 <= i < k の範囲、 k <= i < n の範囲でそれぞれソートされている必要があります。
     */
    template <std::ranges::bidirectional_range Kinds, typename Comp = std::ranges::less, typename Proj = std::identity>
        requires std::sortable<std::ranges::iterator_t<Kinds>, Comp, Proj>
    constexpr bool next_combination(Kinds&& kinds, std::size_t k, Comp comp = {}, Proj proj = {})
    {
        auto first = std::ranges::begin(kinds);
        auto last = std::ranges::end(kinds);

        return next_combination(first, last, k, comp, proj);
    }

}