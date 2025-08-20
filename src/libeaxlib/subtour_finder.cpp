#include "subtour_finder.hpp"

#include "eaxdef.hpp"

namespace eax {
void SubtourFinder::construct_segments(std::vector<std::tuple<size_t, size_t, size_t>>& cut_positions,
                        eax::SubtourList& subtour_list,
                        std::vector<size_t>& pos_to_segment_id) {
    
    auto& segments = subtour_list.segments;

    segments.reserve(cut_positions.size());
    auto add_segment = [&segments, &cut_positions, &pos_to_segment_id](size_t begin_i, size_t end_i) {
        auto [beginning_pos, beginning_adjacent_pos, beginning_minus_one_adjacent_pos] = cut_positions[begin_i];
        auto [end_pos_plus_one, end_pos_plus_one_adjacent_pos, end_adjacent_pos] = cut_positions[end_i];
        eax::SubtourList::Segment segment {
            .ID = segments.size(),
            .beginning_pos = beginning_pos,
            .end_pos = end_pos_plus_one - 1,
            .beginning_adjacent_pos = beginning_adjacent_pos,
            .end_adjacent_pos = end_adjacent_pos,
            .sub_tour_ID = eax::SubtourList::Segment::NULL_SUB_TOUR_ID
        };
        pos_to_segment_id[beginning_pos] = segment.ID;
        pos_to_segment_id[end_pos_plus_one - 1] = segment.ID;
        segments.push_back(segment);
    };

    for (size_t i = 0; i < cut_positions.size() - 1; ++i) {
        add_segment(i, i + 1);
    }
}

/**
 * @return the number of sub-tours
 */
size_t SubtourFinder::set_sub_tour_ids(eax::SubtourList& subtour_list,
                      std::vector<size_t>& pos_to_segment_id) {
    auto& segments = subtour_list.segments;

    // sub_tour_ID を設定
    size_t sub_tour_id = 0;
    constexpr size_t NULL_SUB_TOUR_ID = eax::SubtourList::Segment::NULL_SUB_TOUR_ID;
    while (true) {
        size_t found_segment = NULL_SUB_TOUR_ID;
        for (size_t i = 0; i < segments.size(); ++i) {
            if (segments[i].sub_tour_ID == NULL_SUB_TOUR_ID) {
                found_segment = i;
                break;
            }
        }
        if (found_segment == NULL_SUB_TOUR_ID) {
            break;
        }

        size_t current_segment = found_segment;
        while(true) {
            segments[current_segment].sub_tour_ID = sub_tour_id;
            size_t next_segment = pos_to_segment_id[segments[current_segment].beginning_adjacent_pos];

            if (segments[next_segment].sub_tour_ID != NULL_SUB_TOUR_ID) {
                next_segment = pos_to_segment_id[segments[current_segment].end_adjacent_pos];
                if (segments[next_segment].sub_tour_ID != NULL_SUB_TOUR_ID) {
                    // 1周した
                    break;
                }
            }
            
            current_segment = next_segment;
        } 
        
        sub_tour_id++;
    }
    
    return sub_tour_id;
}

void SubtourFinder::calc_sub_tour_sizes_and_merge_redundant_segments(eax::SubtourList& subtour_list,
                                                     size_t sub_tour_count) {
    
    auto& segments = subtour_list.segments;
    auto& sub_tour_sizes = subtour_list.sub_tour_sizes;

    sub_tour_sizes.resize(sub_tour_count, 0);

    size_t forcused_segment_id = 0;
    for (const auto& segment : segments) {
        if (segment.sub_tour_ID == segments[forcused_segment_id].sub_tour_ID) {
            segments[forcused_segment_id].end_pos = segment.end_pos;
            segments[forcused_segment_id].end_adjacent_pos = segment.end_adjacent_pos;
            sub_tour_sizes[segment.sub_tour_ID] += segment.end_pos - segment.beginning_pos + 1;
        } else {
            ++forcused_segment_id;
            segments[forcused_segment_id] = segment;
            sub_tour_sizes[segment.sub_tour_ID] = segment.end_pos - segment.beginning_pos + 1;
        }
    }
    segments.resize(forcused_segment_id + 1);
}

}