#pragma once

#include <string>
#include <vector>
#include <cmath>

namespace tsp {
    namespace distance {
        inline double EUC_2D(double x1, double y1, double x2, double y2) {
            return size_t(std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) + 0.5);
        }
        
        inline double ATT(double x1, double y1, double x2, double y2) {
            double rij = std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
            double tij = int(rij + 0.5);
            if (tij < rij) return tij + 1.0;
            else return tij;
        }
    }

    struct TSP {
        std::string name;
        std::string distance_type;
        size_t city_count;
        std::vector<std::vector<double>> adjacency_matrix;
    };

    class TSP_Loader {
        public:
            TSP_Loader() = default;
            ~TSP_Loader() = default;

            static TSP load_tsp(const std::string& file_name);
        private:
    };
}