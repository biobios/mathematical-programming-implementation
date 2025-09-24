#pragma once

#include <chrono>

#include "context.hpp"

namespace eax {
auto execute_ga(std::vector<Individual>& population, Context& context, std::chrono::system_clock::time_point timeout_time) {
    using namespace std;
    using Context = eax::Context;
    // オブジェクトプール
    eax::ObjectPools object_pools(context.env.tsp.city_count);
    
    // 交叉関数
    eax::EAX_N_AB eax_n_ab(object_pools);
    eax::EAX_Block2 eax_block2(object_pools);
    auto crossover_func = [&eax_n_ab, &eax_block2](const Individual& parent1, const Individual& parent2,
                                Context& context) {
        auto& env = context.env;
        switch (context.eax_type) {
            case eax::EAXType::One_AB:
                return eax_n_ab(parent1, parent2, env.num_children, env.tsp, context.random_gen, 1);
            case eax::EAXType::Block2:
                return eax_block2(parent1, parent2, env.num_children, env.tsp, context.random_gen);
            default:
                throw std::runtime_error("Unknown EAX type.");
        }
    };

    // 適応度関数
    auto calc_fitness_lambda = [](const eax::CrossoverDelta& child, Context& context) {
        auto& env = context.env;
        switch (env.selection_type) {
            case eax::SelectionType::Greedy:
                return eax::eval::delta::Greedy()(child, env.tsp.adjacency_matrix);
            case eax::SelectionType::Ent:
                return eax::eval::delta::Entropy()(child, env.tsp.adjacency_matrix, context.pop_edge_counts, env.population_size);
            case eax::SelectionType::DistancePreserving:
                return eax::eval::delta::DistancePreserving()(child, env.tsp.adjacency_matrix, context.pop_edge_counts);
            default:
                throw std::runtime_error("Unknown selection type");
        }
    };
    
    // 更新処理関数
    struct {
        std::chrono::system_clock::time_point timeout_time;
        mpi::genetic_algorithm::TerminationReason operator()(vector<Individual>& population, Context& context, size_t generation) {
            context.current_generation = generation;

            // Greedy Selection以外はエッジカウントを個体の評価に使用するため、個体更新時にエッジカウントも更新する
            if (context.env.selection_type != eax::SelectionType::Greedy) {
                update_individual_and_edge_counts(population, context);
            } else {
                update(population, context);
            }
            
            if (std::chrono::system_clock::now() >= timeout_time) {
                return mpi::genetic_algorithm::TerminationReason::TimeLimit;
            }

            return continue_condition(population, context, generation);
        }
        
        void update(vector<Individual>& population, Context& context) {
            for (auto& individual : population) {
                individual.update(context.env.tsp.adjacency_matrix);
            }
        }
        
        void update_individual_and_edge_counts(vector<Individual>& population, Context& context) {
            for (auto& individual : population) {
                auto delta = individual.update(context.env.tsp.adjacency_matrix);
                for (const auto& mod : delta.get_modifications()) {
                    size_t v1 = mod.edge1.first;
                    size_t v2 = mod.edge1.second;
                    size_t new_v2 = mod.new_v2;
                    context.pop_edge_counts[v1][v2] -= 1;
                    context.pop_edge_counts[v1][new_v2] += 1;
                }
            }
        }

        mpi::genetic_algorithm::TerminationReason continue_condition(const vector<Individual>& population, Context& context, size_t generation) {
            double best_length = std::numeric_limits<double>::max();
            double average_length = 0.0;
            for (size_t i = 0; i < population.size(); ++i) {
                double length = population[i].get_distance();
                best_length = std::min(best_length, length);
                average_length += length;
            }
            average_length /= population.size();
            
            if (best_length < context.best_length) {
                context.best_length = best_length;
                context.generation_of_reached_best = generation;
                context.stagnation_generations = 0;
            }else {
                context.stagnation_generations += 1;
            }
            
            if (average_length - best_length < 0.001)
                return mpi::genetic_algorithm::TerminationReason::Converged; // 収束条件
            
            const size_t N_child = context.env.num_children;
            
            if (context.stage == Context::GA_Stage::Stage1) {
                if (context.G_devided_by_10 == 0 && context.stagnation_generations >= (1500 / N_child)) {
                    context.G_devided_by_10 = generation / 10;
                } else if (context.G_devided_by_10 > 0 && context.stagnation_generations >= context.G_devided_by_10) {
                    context.stage = Context::GA_Stage::Stage2;
                    context.eax_type = eax::EAXType::Block2;
                    context.stagnation_generations = 0;
                    context.generation_of_transition_to_stage2 = generation;
                    context.G_devided_by_10 = 0;
                }
            } else {
                if (context.G_devided_by_10 == 0 && context.stagnation_generations >= (1500 / N_child)) {
                    context.G_devided_by_10 = (generation - context.generation_of_transition_to_stage2) / 10;
                } else if (context.G_devided_by_10 > 0 && context.stagnation_generations >= context.G_devided_by_10) {
                    return mpi::genetic_algorithm::TerminationReason::Stagnation; // 停滞条件
                }
            }
            
            return mpi::genetic_algorithm::TerminationReason::NotTerminated;
        }
    } update_func {timeout_time};
    
    // ロガー
    struct {
        void operator()([[maybe_unused]]const vector<Individual>& population, Context& context, size_t generation) {
            std::vector<double> lengths(population.size());
            for (size_t i = 0; i < population.size(); ++i) {
                lengths[i] = population[i].get_distance();
            }
            double best_length = *std::min_element(lengths.begin(), lengths.end());
            double average_length = std::accumulate(lengths.begin(), lengths.end(), 0.0) / lengths.size();
            double worst_length = *std::max_element(lengths.begin(), lengths.end());
            cout << "Generation " << generation << ": Best Length = " << best_length 
                    << ", Average Length = " << average_length << ", Worst Length = " << worst_length << endl;
            
            if (context.start_time.time_since_epoch().count() == 0) {
                // 計測開始時刻が未設定なら、現在時刻を設定
                const_cast<Context&>(context).start_time = std::chrono::system_clock::now();
            } else {
                auto now = std::chrono::system_clock::now();
                context.elapsed_time += std::chrono::duration<double>(now - context.start_time).count();
                const_cast<Context&>(context).start_time = now;
            }
        }
    } logging;
    
    struct {
        void operator()([[maybe_unused]]const vector<Individual>& population, Context& context, size_t generation, [[maybe_unused]]mpi::genetic_algorithm::TerminationReason reason) {
            context.final_generation = generation;
        }
    } post_process;

    // 世代交代処理
    eax::GenerationalStep generational_step(calc_fitness_lambda, crossover_func);
    
    // GA実行オブジェクト
    eax::GenerationalModel genetic_algorithm(generational_step, update_func, logging, post_process);

    return genetic_algorithm.execute(population, context, context.current_generation);
}
Context create_context(const std::vector<Individual>& initial_population, Environment const& env) {
    Context context;
    context.env = env;

    context.eax_type = EAXType::One_AB;
    context.set_initial_edge_counts(initial_population);
    context.random_gen = std::mt19937(env.random_seed);
    return context;   
}

// 人間が読める形式でContextを出力
void serialize_context(const Context& context, std::ostream& os) {
    os << "# Environment" << std::endl;
    os << "## TSP" << std::endl;
    os << "name=" << context.env.tsp.name << std::endl;
    // デシリアライズの時にTSPファイルを入力させるので、そのほかの情報は出力しない
    os << "## Other Parameters" << std::endl;
    os << "population_size=" << context.env.population_size << std::endl;
    os << "num_children=" << context.env.num_children << std::endl;
    os << "selection_type=";
    switch (context.env.selection_type) {
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
    os << "random_seed=" << context.env.random_seed << std::endl;
    os << "# GA State" << std::endl;
    os << "eax_type=";
    switch (context.eax_type) {
        case EAXType::One_AB:
            os << "N_AB" << std::endl;
            break;
        case EAXType::Block2:
            os << "Block2" << std::endl;
            break;
        default:
            os << "Unknown" << std::endl;
            break;
    }
    os << "## Population Edge Counts" << std::endl;
    for (const auto& row : context.pop_edge_counts) {
        for (const auto& count : row) {
            os << count << " ";
        }
        os << std::endl;
    }
    os << "## Random Generator State" << std::endl;
    os << context.random_gen << std::endl;
    os << "## Other State Variables" << std::endl;
    os << "best_length=" << context.best_length << std::endl;
    os << "generation_of_reached_best=" << context.generation_of_reached_best << std::endl;
    os << "stagnation_generations=" << context.stagnation_generations << std::endl;
    os << "generation_of_transition_to_stage2=" << context.generation_of_transition_to_stage2 << std::endl;
    os << "G_devided_by_10=" << context.G_devided_by_10 << std::endl;
    os << "current_generation=" << context.current_generation << std::endl;
    os << "final_generation=" << context.final_generation << std::endl;
    os << "stage=";
    switch (context.stage) {
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
    os << "elapsed_time=" << context.elapsed_time << std::endl;
}

Context deserialize_context(std::istream& is, tsp::TSP tsp) {
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

    // # GA State
    read_val("# GA State");
    // eax_type=...
    std::string eax_type_str = read_val("eax_type=");
    if (eax_type_str == "N_AB") {
        context.eax_type = EAXType::One_AB;
    } else if (eax_type_str == "Block2") {
        context.eax_type = EAXType::Block2;
    } else {
        throw std::runtime_error("Unknown EAX type: " + eax_type_str);
    }
    // ## Population Edge Counts
    read_val("## Population Edge Counts");
    context.pop_edge_counts.resize(context.env.tsp.city_count, std::vector<size_t>(context.env.tsp.city_count, 0));
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

void serialize_population(const std::vector<Individual>& population, std::ostream& os) {
    os << "# Population" << std::endl;
    for (const auto& individual : population) {
        individual.serialize(os);
        os << std::endl;
    }
}

std::vector<Individual> deserialize_population(std::istream& is) {
    std::vector<Individual> population;
    std::string line;
    // # Population
    std::getline(is, line);
    if (line != "# Population") throw std::runtime_error("Expected '# Population'");
    while (std::getline(is, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        Individual individual = Individual::deserialize(iss);
        population.push_back(individual);
    }
    return population;
}
}