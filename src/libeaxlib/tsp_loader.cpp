#include "tsp_loader.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace tsp {
    TSP TSP_Loader::load_tsp(const std::string& file_name) {
        TSP tsp;
        std::ifstream file(file_name);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open the file: " + file_name);
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.find("NAME") != std::string::npos) {
                std::istringstream iss(line);
                std::string name_label;
                std::string colon;
                iss >> name_label >> colon >> tsp.name;
            } else if (line.find("DIMENSION") != std::string::npos) {
                std::istringstream iss(line);
                std::string dimension_label;
                std::string colon;
                iss >> dimension_label >> colon >> tsp.city_count;

                tsp.adjacency_matrix.resize(tsp.city_count, std::vector<int64_t>(tsp.city_count, 0.0));
            } else if (line.find("EDGE_WEIGHT_TYPE") != std::string::npos) {
                std::istringstream iss(line);
                std::string edge_weight_type_label;
                std::string colon;
                iss >> edge_weight_type_label >> colon >> tsp.distance_type;
            } else if (line.find("NODE_COORD_SECTION") != std::string::npos) {
                break; // Start reading the adjacency matrix
            }
        }
        
        std::vector<std::pair<double, double>> coordinates(tsp.city_count);
        size_t city_index = 0;
        while (std::getline(file, line)) {
            if (line.find("EOF") != std::string::npos) {
                break; // End of the coordinates section
            }
            std::istringstream iss(line);
            size_t id;
            double x, y;
            if (iss >> id >> x >> y) {
                if (id < 1 || id > tsp.city_count) {
                    throw std::runtime_error("City ID out of range: " + std::to_string(id));
                }
                coordinates[id - 1] = {x, y};
                city_index++;
            } else {
                throw std::runtime_error("Invalid line format: " + line);
            }
        }

        if (city_index != tsp.city_count) {
            throw std::runtime_error("Number of cities does not match the specified dimension.");
        }
        
        
        // Fill the adjacency matrix based on the coordinates
        for (size_t i = 0; i < tsp.city_count; ++i) {
            for (size_t j = 0; j < tsp.city_count; ++j) {
                if (i == j) {
                    tsp.adjacency_matrix[i][j] = 0; // Distance to itself is 0
                } else {
                    double x1 = coordinates[i].first;
                    double y1 = coordinates[i].second;
                    double x2 = coordinates[j].first;
                    double y2 = coordinates[j].second;
                    if (tsp.distance_type == "EUC_2D") {
                        tsp.adjacency_matrix[i][j] = distance::EUC_2D(x1, y1, x2, y2);
                    } else if (tsp.distance_type == "ATT") {
                        tsp.adjacency_matrix[i][j] = distance::ATT(x1, y1, x2, y2);
                    } else {
                        throw std::runtime_error("Unsupported distance type: " + tsp.distance_type);
                    }
                }
            }
        }

        // Construct the nearest neighbor list
        tsp.NN_list.resize(tsp.city_count);
        for (size_t i = 0; i < tsp.city_count; ++i) {
            tsp.NN_list[i].resize(tsp.city_count - 1);

            for (size_t j = 0; j < i; ++j) {
                tsp.NN_list[i][j] = j;
            }
            for (size_t j = i + 1; j < tsp.city_count; ++j) {
                tsp.NN_list[i][j - 1] = j;
            }
        }

        for (size_t i = 0; i < tsp.city_count; ++i) {
            std::sort(tsp.NN_list[i].begin(), tsp.NN_list[i].end(),
                      [&tsp, i](size_t a, size_t b) {
                          return tsp.adjacency_matrix[i][a] < tsp.adjacency_matrix[i][b];
                      });
        }

        file.close();
        return tsp;
    }
}