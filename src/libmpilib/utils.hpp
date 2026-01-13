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

    template <typename T>
    struct is_tuple_like : std::false_type {};

    template <typename... Ts>
    struct is_tuple_like<std::tuple<Ts...>> : std::true_type {};

    template <typename First, typename Second>
    struct is_tuple_like<std::pair<First, Second>> : std::true_type {};

    template <typename T, std::size_t N>
    struct is_tuple_like<std::array<T, N>> : std::true_type {};

    template <typename I, typename S, std::ranges::subrange_kind K>
    struct is_tuple_like<std::ranges::subrange<I, S, K>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_tuple_like_v = is_tuple_like<std::remove_cvref_t<T>>::value;

    template <typename T>
    concept tuple_like = is_tuple_like_v<T>;
}