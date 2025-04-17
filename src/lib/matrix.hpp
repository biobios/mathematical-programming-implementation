#pragma once
#include <cstdint>
#include <array>

template <std::size_t Rows, std::size_t Cols, typename ValueType=double>
class Matrix {
    public:
    private:
        std::array<std::array<ValueType, Cols>, Rows> data;       
};