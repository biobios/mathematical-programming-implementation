#pragma once

#include <vector>
#include <random>
#include <ranges>

#include "individual.hpp"
#include "tsp_loader.hpp"

namespace eax {
class Environment;

struct Segment {
    size_t ID;
    size_t beginning_pos;
    size_t end_pos;
    size_t beginning_adjacent_pos;
    size_t end_adjacent_pos;
    size_t sub_tour_ID = std::numeric_limits<size_t>::max();
};

using ABCycle = std::vector<size_t>;

class IntermediateIndividual {
public:
    IntermediateIndividual() = default;
    IntermediateIndividual(const eax::Individual& parent) : working_individual(parent) {}
    eax::Child convert_to_child_and_revert();
    void assign(const eax::Individual& parent);
    void swap_edges(std::pair<size_t, size_t> edge1, std::pair<size_t, size_t> edge2);    
    void change_connection(size_t v1, size_t v2, size_t new_v2);
    template <std::ranges::range ABCycles>
        requires std::convertible_to<std::ranges::range_value_t<ABCycles>, const ABCycle&>
    void apply_AB_cycles(const ABCycles& AB_cycles,
                         const std::vector<size_t>& pos,
                         Environment& env);
    size_t sub_tour_count() const;    
    std::pair<size_t, size_t> find_min_size_sub_tour() const;    
    size_t find_sub_tour_containing(size_t pos) const;    
    size_t get_city_pos_of_sub_tour(size_t sub_tour_id) const;    
    void merge_sub_tour(size_t sub_tour_id1, size_t sub_tour_id2);    
    const std::array<size_t, 2>& operator[](size_t index);    
    const std::array<size_t, 2>& operator[](size_t index) const;
private:
    void revert();    
    void reset();
    void undo(const eax::Child::Modification& modification);
    eax::Individual working_individual;
    std::vector<eax::Child::Modification> modifications;
    std::vector<Segment> segments;
    std::vector<size_t> sub_tour_sizes;
};
    
std::vector<Child> edge_assembly_crossover(const Individual& parent1, const Individual& parent2, size_t children_size,
                                            Environment& env, std::mt19937& rng);

void print_time();
}