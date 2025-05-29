#include <array>
#include <chrono>
#include <iostream>
#include <ranges>

#include "matrix.hpp"
#include "simplex.hpp"

int main()
{

    mpi::Matrix<3, 1> c = {{-2}, {-1}, {-1}};
    mpi::Matrix<2, 3> A = {{1, 2, 0}, {1, 4, 2}};
    mpi::Matrix<2, 1> b = {{12}, {20}};

    auto result = mpi::linear_programming::SimplexTableau()(c, A, b);
    if(result.has_value())
    {
        std::cout << "Optimal solution" << std::endl;
        std::cout << "x = ";
        for (std::size_t i = 0; i < 3; ++i)
        {
            std::cout << result.value().at(i, 0) << " ";
        }
        std::cout << std::endl;

        auto objective_value = c.transpose() * result.value();
        std::cout << "Objective value = " << objective_value << std::endl;
    }
    else
    {
        std::cout << "No solution" << std::endl;
        if(result.error() == mpi::linear_programming::LPNoSolutionReason::Infeasible)
        {
            std::cout << "Infeasible" << std::endl;
        }
        else if(result.error() == mpi::linear_programming::LPNoSolutionReason::Unbounded)
        {
            std::cout << "Unbounded" << std::endl;
        }
    }

    return 0;
}