#pragma once

#include <vector>
#include <utility>
#include <chrono>
#include <iostream>

#include "genetic_algorithm.hpp"

#include "individual.hpp"
#include "context.hpp"

namespace eax {
std::pair<mpi::genetic_algorithm::TerminationReason, std::vector<Individual>> execute_ga(
    std::vector<Individual>& population,
    Context& context,
    std::chrono::system_clock::time_point timeout_time,
    const std::string& log_file_name);
Context create_context(const std::vector<Individual>& initial_population, Environment const& env);
void serialize_population(const std::vector<Individual>& population, std::ostream& os);
std::vector<Individual> deserialize_population(std::istream& is);
}