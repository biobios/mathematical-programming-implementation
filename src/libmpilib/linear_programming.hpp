#pragma once

namespace mpi
{
    namespace linear_programming
    {
        enum class LPNoSolutionReason
        {
            Infeasible,
            Unbounded
        };
    }
}