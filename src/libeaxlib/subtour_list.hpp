#pragma once

#include <cstddef>
#include <vector>
#include <limits>

namespace eax {
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

    size_t sub_tour_count() const;
    std::pair<size_t, size_t> find_min_size_sub_tour() const;
    size_t find_sub_tour_containing(size_t pos) const;
    size_t get_city_pos_of_sub_tour(size_t sub_tour_id) const;
    void merge_sub_tour(size_t sub_tour_id1, size_t sub_tour_id2);
    void clear();
};
}