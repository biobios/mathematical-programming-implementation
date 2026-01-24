#pragma once

#include <vector>
#include <utility>
#include <chrono>
#include <iostream>

#include "genetic_algorithm.hpp"

#include "context.hpp"
#include "individual_with_pending_delta.hpp"

namespace eax {

std::pair<mpi::genetic_algorithm::TerminationReason, std::vector<Individual>> execute_ga(
    std::vector<Individual>& population,
    Context& context,
    const std::string& log_file_name);

Context create_context(const std::vector<Individual>& initial_population, Environment const& env);
}