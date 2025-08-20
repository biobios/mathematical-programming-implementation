#include "subtour_list.hpp"

#include <limits>
#include <stdexcept>

namespace eax {
size_t SubtourList::sub_tour_count() const {
    return sub_tour_sizes.size();
}

std::pair<size_t, size_t> SubtourList::find_min_size_sub_tour() const {
    size_t min_size = std::numeric_limits<size_t>::max();
    size_t min_size_sub_tour_id = 0;
    for (size_t i = 0; i < sub_tour_sizes.size(); ++i) {
        if (sub_tour_sizes[i] < min_size) {
            min_size = sub_tour_sizes[i];
            min_size_sub_tour_id = i;
        }
    }
    return {min_size_sub_tour_id, min_size};
}

size_t SubtourList::find_sub_tour_containing(size_t pos) const {
    for (const auto& segment : segments) {
        if (segment.beginning_pos <= pos && pos <= segment.end_pos) {
            return segment.sub_tour_ID;
        }
    }
    throw std::runtime_error("No sub-tour found containing the position");
}

size_t SubtourList::get_city_pos_of_sub_tour(size_t sub_tour_id) const {
    for (const auto& segment : segments) {
        if (segment.sub_tour_ID == sub_tour_id) {
            return segment.beginning_pos;
        }
    }
    throw std::runtime_error("No sub-tour found with the given ID");
}

void SubtourList::merge_sub_tour(size_t sub_tour_id1, size_t sub_tour_id2) {
    for (auto& segment : segments) {
        if (segment.sub_tour_ID == sub_tour_id2) {
            segment.sub_tour_ID = sub_tour_id1;
        }
    }
    
    sub_tour_sizes[sub_tour_id1] += sub_tour_sizes[sub_tour_id2];
    size_t last_index = sub_tour_sizes.size() - 1;
    if (sub_tour_id2 < last_index) {
        for (auto& segment : segments) {
            if (segment.sub_tour_ID == last_index) {
                segment.sub_tour_ID = sub_tour_id2;
            }
        }

        sub_tour_sizes[sub_tour_id2] = sub_tour_sizes[last_index];
    }
    
    sub_tour_sizes.pop_back();
}

void SubtourList::clear() {
    segments.clear();
    sub_tour_sizes.clear();
}

}