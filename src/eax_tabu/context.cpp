#include "context.hpp"

#include "eax_tag.hpp"

namespace eax {
// 人間が読める形式でContextを出力
void Context::serialize(std::ostream& os) const {
    os << "# Environment" << std::endl;
    os << "## TSP" << std::endl;
    os << "name=" << env.tsp.name << std::endl;
    // デシリアライズの時にTSPファイルを入力させるので、そのほかの情報は出力しない
    os << "## Other Parameters" << std::endl;
    os << "population_size=" << env.population_size << std::endl;
    os << "num_children=" << env.num_children << std::endl;
    os << "selection_type=";
    switch (env.selection_type) {
        case SelectionType::Greedy:
            os << "Greedy" << std::endl;
            break;
        case SelectionType::Ent:
            os << "Ent" << std::endl;
            break;
        case SelectionType::DistancePreserving:
            os << "DistancePreserving" << std::endl;
            break;
        default:
            os << "Unknown" << std::endl;
            break;
    }
    os << "random_seed=" << env.random_seed << std::endl;
    os << "eax_type=";
    os << env.eax_type << std::endl;

    os << "# GA State" << std::endl;
    os << "## Population Edge Counts" << std::endl;
    for (const auto& row : pop_edge_counts) {
        for (const auto& count : row) {
            os << count << " ";
        }
        os << std::endl;
    }
    os << "## Random Generator State" << std::endl;
    os << random_gen << std::endl;
    os << "## Other State Variables" << std::endl;
    os << "best_length=" << best_length << std::endl;
    os << "generation_of_reached_best=" << generation_of_reached_best << std::endl;
    os << "stagnation_generations=" << stagnation_generations << std::endl;
    os << "current_generation=" << current_generation << std::endl;
    os << "final_generation=" << final_generation << std::endl;
    os << "elapsed_time=" << elapsed_time << std::endl;
}

Context Context::deserialize(std::istream& is, tsp::TSP tsp) {
    Context context;
    context.env.tsp = std::move(tsp);
    
    auto read_val = [&is](const std::string& prefix) {
        std::string line;
        std::getline(is, line);
        if (line.rfind(prefix, 0) != 0) {
            throw std::runtime_error("Expected '" + prefix + "', got: " + line);
        }
        return line.substr(prefix.size());
    };

    std::string line;
    // # Environment
    read_val("# Environment");

    // ## TSP
    read_val("## TSP");

    // name=...
    std::string tsp_name = read_val("name=");
    if (tsp_name != context.env.tsp.name) {
        throw std::runtime_error("TSP name mismatch: expected " + context.env.tsp.name + ", got " + tsp_name);
    }

    // ## Other Parameters
    read_val("## Other Parameters");

    // population_size=...
    context.env.population_size = std::stoul(read_val("population_size="));

    // num_children=...
    context.env.num_children = std::stoul(read_val("num_children="));

    // selection_type=...
    std::string selection_type_str = read_val("selection_type=");
    if (selection_type_str == "Greedy") {
        context.env.selection_type = SelectionType::Greedy;
    } else if (selection_type_str == "Ent") {
        context.env.selection_type = SelectionType::Ent;
    } else if (selection_type_str == "DistancePreserving") {
        context.env.selection_type = SelectionType::DistancePreserving;
    } else {
        throw std::runtime_error("Unknown selection type: " + selection_type_str);
    }

    // random_seed=...
    context.env.random_seed = static_cast<std::mt19937::result_type>(std::stoull(read_val("random_seed=")));

    // eax_type=...
    std::string eax_type_str = read_val("eax_type=");
    context.env.eax_type = eax::create_eax_tag_from_string<eax_type_t>(eax_type_str);

    // # GA State
    read_val("# GA State");
    // ## Population Edge Counts
    read_val("## Population Edge Counts");
    context.pop_edge_counts.assign(context.env.tsp.city_count, std::vector<size_t>(context.env.tsp.city_count, 0));
    for (size_t i = 0; i < context.env.tsp.city_count; ++i) {
        std::getline(is, line);
        std::istringstream iss(line);
        for (size_t j = 0; j < context.env.tsp.city_count; ++j) {
            if (!(iss >> context.pop_edge_counts[i][j])) {
                throw std::runtime_error("Error reading edge counts");
            }
        }
    }
    // ## Random Generator State
    read_val("## Random Generator State");
    is >> context.random_gen;
    std::getline(is, line); // consume the rest of the line after reading the generator state

    // ## Other State Variables
    read_val("## Other State Variables");

    // best_length=...
    context.best_length = std::stoull(read_val("best_length="));

    // generation_of_reached_best=...
    context.generation_of_reached_best = std::stoul(read_val("generation_of_reached_best="));

    // stagnation_generations=...
    context.stagnation_generations = std::stoul(read_val("stagnation_generations="));

    // current_generation=...
    context.current_generation = std::stoul(read_val("current_generation="));

    // final_generation=...
    context.final_generation = std::stoul(read_val("final_generation="));

    // elapsed_time=...
    context.elapsed_time = std::stod(read_val("elapsed_time="));

    return context;
}
}