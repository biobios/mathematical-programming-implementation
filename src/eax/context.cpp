#include "context.hpp"

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
    struct {
        std::ostream& os;
        void operator()(const EAXType& type) {
            switch (type) {
                case EAXType::EAX_Rand:
                    os << "EAX_Rand" << std::endl;
                    break;
                case EAXType::Block2:
                    os << "Block2" << std::endl;
                    break;
                default:
                    os << "Unknown" << std::endl;
                    break;
            }
        }
        void operator()(const EAX_N_AB& n_ab) {
            os << "EAX_" << n_ab.N << "_AB" << std::endl;
        }
    } visitor {os};
    std::visit(visitor, env.eax_type);

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
    os << "generation_of_transition_to_stage2=" << generation_of_transition_to_stage2 << std::endl;
    os << "G_devided_by_10=" << G_devided_by_10 << std::endl;
    os << "current_generation=" << current_generation << std::endl;
    os << "final_generation=" << final_generation << std::endl;
    os << "stage=";
    switch (stage) {
        case Context::GA_Stage::Stage1:
            os << "Stage1" << std::endl;
            break;
        case Context::GA_Stage::Stage2:
            os << "Stage2" << std::endl;
            break;
        default:
            os << "Unknown" << std::endl;
            break;
    }
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
    if (eax_type_str == "EAX_Rand") {
        context.env.eax_type = EAXType::EAX_Rand;
    } else if (eax_type_str == "Block2") {
        context.env.eax_type = EAXType::Block2;
    } else if (EAX_N_AB::is_EAX_N_AB(eax_type_str)) {
        context.env.eax_type = EAX_N_AB(eax_type_str);
    } else {
        throw std::runtime_error("Unknown EAX type: " + eax_type_str);
    }

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

    // generation_of_transition_to_stage2=...
    context.generation_of_transition_to_stage2 = std::stoul(read_val("generation_of_transition_to_stage2="));

    // G_devided_by_10=...
    context.G_devided_by_10 = std::stoul(read_val("G_devided_by_10="));
    
    // current_generation=...
    context.current_generation = std::stoul(read_val("current_generation="));

    // final_generation=...
    context.final_generation = std::stoul(read_val("final_generation="));

    // stage=...
    std::string stage_str = read_val("stage=");
    if (stage_str == "Stage1") {
        context.stage = Context::GA_Stage::Stage1;
    } else if (stage_str == "Stage2") {
        context.stage = Context::GA_Stage::Stage2;
    } else {
        throw std::runtime_error("Unknown GA stage: " + stage_str);
    }
    
    // elapsed_time=...
    context.elapsed_time = std::stod(read_val("elapsed_time="));

    return context;
}
}