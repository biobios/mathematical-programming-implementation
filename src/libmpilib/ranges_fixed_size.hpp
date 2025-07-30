#pragma once

#include <concepts>
#include <array>
#include <span>
#include <type_traits>
#include <ranges>

#include "exception_helper.hpp"

namespace mpi
{
    namespace ranges
    {
        template <std::ranges::forward_range R, std::size_t N>
        class fixed_size_view : public std::ranges::view_interface<fixed_size_view<R, N>>
        {
            public:
                fixed_size_view(R&& r, std::size_t begin = 0){
                    auto end = std::ranges::end(r);
                    begin_ = std::ranges::next(r.begin(), begin, end);
                    end_ = std::ranges::next(begin_, N, end);

                    if (std::ranges::distance(begin_, end_) != N)
                        exception::throw_exception<std::out_of_range>("Range size is not equal to N");
                }
                static constexpr std::size_t size() noexcept { return N; };

                friend constexpr auto begin(fixed_size_view& v) noexcept
                {
                    return v.begin_;
                }

                friend constexpr auto end(fixed_size_view& v) noexcept
                {
                    return v.end_;
                }

                friend constexpr auto begin(const fixed_size_view& v) noexcept
                {
                    return v.begin_;
                }

                friend constexpr auto end(const fixed_size_view& v) noexcept
                {
                    return v.end_;
                }
                
            private:
                std::ranges::iterator_t<R> begin_;
                std::ranges::iterator_t<R> end_;
        };

        inline namespace fixed_size_cpo
        {
            template <std::size_t N>
            struct fixed_size_adapter_closure /*: public std::ranges::range_adaptor_closure<fixed_size_adapter_closure<N>>*/
            {
                    constexpr fixed_size_adapter_closure(std::size_t begin = 0) : begin_(begin) {}

                    template <std::ranges::viewable_range R>
                    constexpr auto operator()(R&& r) const
                    {
                        return fixed_size_view<R, N>(std::forward<R>(r), begin_);
                    }
                private:
                    std::size_t begin_ = 0;
            };

            template <std::size_t N>
            struct fixed_size_adapter
            {
                template <std::ranges::viewable_range R>
                constexpr auto operator()(R&& r, std::size_t begin = 0) const
                {
                    return fixed_size_adapter_closure<N>(begin)(std::forward<R>(r));
                }

                constexpr auto operator()(std::size_t begin = 0) const
                {
                    return fixed_size_adapter_closure<N>(begin);
                }
            };
        }

        template <std::size_t N>
        inline constexpr fixed_size_adapter<N> fixed_size{};

        template <typename T>
        struct is_array_class : public std::false_type {};

        template <typename T, std::size_t N>
        struct is_array_class<std::array<T, N>> : public std::true_type {};
    
        template <typename T>
        struct is_span_class : public std::false_type {};

        template <typename T>
        struct is_span_class<std::span<T>> : public std::false_type {};

        template <typename T, std::size_t N>
        struct is_span_class<std::span<T, N>> : public std::true_type {};

        template <typename T>
        struct is_fixed_size_view : public std::false_type {};

        template <typename R, std::size_t N>
        struct is_fixed_size_view<fixed_size_view<R, N>> : public std::true_type {};

        template <typename T>
        struct is_empty_view : public std::false_type {};

        template <typename T>
        struct is_empty_view<std::ranges::empty_view<T>> : public std::true_type {};

        template <typename T>
        concept fixed_size_range = std::is_bounded_array_v<std::remove_reference_t<T>> || is_array_class<std::remove_cvref_t<T>>::value ||
                                    is_span_class<std::remove_cvref_t<T>>::value || is_fixed_size_view<std::remove_cvref_t<T>>::value ||
                                    is_empty_view<std::remove_cvref_t<T>>::value;
        
        template <typename T>
        struct get_fixed_size_range_size{};

        template <typename T>
            requires is_fixed_size_view<T>::value
        struct get_fixed_size_range_size<T>
        {
            static constexpr size_t value = T::size();
        };

        template <typename T, std::size_t N>
        struct get_fixed_size_range_size<T[N]>
        {
            static constexpr size_t value = N;
        };

        template <typename T, std::size_t N>
        struct get_fixed_size_range_size<std::array<T, N>>
        {
            static constexpr size_t value = N;
        };

        template <typename T, std::size_t N>
            requires (N != std::dynamic_extent)
        struct get_fixed_size_range_size<std::span<T, N>>
        {
            static constexpr size_t value = N;
        };

        template <typename T>
        struct get_fixed_size_range_size<std::ranges::empty_view<T>>
        {
            static constexpr size_t value = 0;
        };

        template <fixed_size_range T>
        constexpr size_t fixed_size_range_size = get_fixed_size_range_size<std::remove_cvref_t<T>>::value;
    }
}