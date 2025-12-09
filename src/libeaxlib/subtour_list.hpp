#pragma once

#include <cstddef>
#include <vector>
#include <limits>

namespace eax {

/**
 * @brief 部分巡回路リストを表す構造体
 */
struct SubtourList {
    struct Segment {
        static constexpr size_t NULL_SUB_TOUR_ID = std::numeric_limits<size_t>::max();
        size_t ID;
        size_t beginning_pos;
        size_t end_pos;
        size_t beginning_adjacent_pos;
        size_t end_adjacent_pos;
        size_t sub_tour_ID; // ID of the sub-tour this segment belongs to
    };

    std::vector<Segment> segments;
    std::vector<size_t> sub_tour_sizes;

    /**
     * @brief 部分巡回路の数を取得する
     * @return 部分巡回路の数
     */
    size_t sub_tour_count() const;
    /**
     * @brief 最小サイズの部分巡回路を見つける
     * @return (部分巡回路ID, サイズ) のペア
     */
    std::pair<size_t, size_t> find_min_size_sub_tour() const;
    /**
     * @brief 指定した位置が属する部分巡回路を見つける
     * @param pos 位置
     * @return 部分巡回路ID
     */
    size_t find_sub_tour_containing(size_t pos) const;
    /**
     * @brief 指定した部分巡回路のある頂点の位置を取得する
     * @param sub_tour_id 部分巡回路ID
     * @return 部分巡回路に属する頂点の位置
     */
    size_t get_city_pos_of_sub_tour(size_t sub_tour_id) const;
    /**
     * @brief 指定した2つの部分巡回路を統合する
     * @param sub_tour_id1 部分巡回路ID1
     * @param sub_tour_id2 部分巡回路ID2
     */
    void merge_sub_tour(size_t sub_tour_id1, size_t sub_tour_id2);
    /**
     * @brief 部分巡回路リストをクリアする
     */
    void clear();
};
}