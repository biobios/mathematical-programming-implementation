#pragma once

#include <string>
#include <vector>
#include <cmath>

namespace tsp {
    namespace distance {
        inline int64_t EUC_2D(double x1, double y1, double x2, double y2) {
            return size_t(std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) + 0.5);
        }
        
        inline int64_t ATT(double x1, double y1, double x2, double y2) {
            double dx = x1 - x2;
            double dy = y1 - y2;
            double rij = std::sqrt((dx * dx + dy * dy) / 10.0);
            int64_t tij = int(rij + 0.5);
            if (tij < rij) return tij + 1;
            else return tij;
        }

        inline int64_t CEIL_2D(double x1, double y1, double x2, double y2) {
            double dx = x1 - x2;
            double dy = y1 - y2;
            return int64_t(std::ceil(std::sqrt(dx * dx + dy * dy)));
        }
    }
    
    using adjacency_matrix_t = std::vector<std::vector<int64_t>>;
    using NN_list_t = std::vector<std::vector<size_t>>;

    struct TSP {
        std::string name;
        std::string distance_type;
        size_t city_count;
        adjacency_matrix_t adjacency_matrix;
        NN_list_t NN_list;
    };

    class TSP_Loader {
        public:
            TSP_Loader() = default;
            ~TSP_Loader() = default;

            static TSP load_tsp(const std::string& file_name);
        private:
    };
}