#pragma once

namespace mpi
{
    struct NOP_Function
    {
        template <typename... Args>
        constexpr void operator()(Args&&...) const noexcept
        {
            // 何もしない
        }
    };
}