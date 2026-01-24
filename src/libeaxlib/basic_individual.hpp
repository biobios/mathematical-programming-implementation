#pragma once

#include "eaxdef.hpp"
#include "checksumed.hpp"

namespace eax {

/**
 * @brief individual_writableの要件を満たす基本的な個体クラス
 */
class BasicIndividual : public Checksumed {
public:
    BasicIndividual(const std::vector<size_t>& path, const adjacency_matrix_t& adjacency_matrix);

    constexpr std::array<size_t, 2>& operator[](size_t index) {
        return doubly_linked_list[index];
    }

    constexpr std::array<size_t, 2> const& operator[](size_t index) const {
        return doubly_linked_list[index];
    }
    
    constexpr size_t size() const {
        return doubly_linked_list.size();
    }

    int64_t get_distance() const {
        return distance;
    }

    void set_distance(int64_t val) {
        distance = val;
    }
private:
    doubly_linked_list_t doubly_linked_list;
    int64_t distance = 0;
};

static_assert(individual_writable<BasicIndividual>);

/**
 * @brief BasicIndividualを内部に持つ読み取り専用個体クラスのミックスイン
 * @tparam T 派生クラスの型
 * @details TはReadableWithBasicIndividual<T>を継承している必要がある。BasicIndividual型のindividualメンバを持つ。
 */
template <typename T>
class ReadableWithBasicIndividual {
public:
    constexpr std::array<size_t, 2> const& operator[](size_t index) const {
        return individual[index];
    }

    constexpr size_t size() const {
        return individual.size();
    }

    uint64_t get_checksum() const {
        return individual.get_checksum();
    }

    int64_t get_distance() const {
        return individual.get_distance();
    }

private:
    friend T;
    BasicIndividual individual;

    ReadableWithBasicIndividual(const std::vector<size_t>& path, const adjacency_matrix_t& adjacency_matrix)
        : individual(path, adjacency_matrix) {}

    ~ReadableWithBasicIndividual() = default;
    ReadableWithBasicIndividual(const ReadableWithBasicIndividual&) = default;
    ReadableWithBasicIndividual(ReadableWithBasicIndividual&&) = default;
    ReadableWithBasicIndividual& operator=(const ReadableWithBasicIndividual&) = default;
    ReadableWithBasicIndividual& operator=(ReadableWithBasicIndividual&&) = default;
};

/**
 * @brief BasicIndividualを内部に持つ読み取り専用個体クラスのミックスイン
 * @tparam T 派生クラスの型 
 * @details TはWritableWithBasicIndividual<T>を継承している必要がある。BasicIndividual型のindividualメンバを持つ。
 */
template <typename T>
class WritableWithBasicIndividual {
public:
    // readable要件
    constexpr std::array<size_t, 2> const& operator[](size_t index) const {
        return individual[index];
    }

    constexpr size_t size() const {
        return individual.size();
    }

    uint64_t get_checksum() const {
        return individual.get_checksum();
    }

    int64_t get_distance() const {
        return individual.get_distance();
    }

    // writable要件
    std::array<size_t, 2>& operator[](size_t index) {
        return individual[index];
    }
    
    void set_checksum(uint64_t val) {
        individual.set_checksum(val);
    }

    void set_distance(int64_t val) {
        individual.set_distance(val);
    }

private:
    friend T;
    BasicIndividual individual;

    WritableWithBasicIndividual(const std::vector<size_t>& path, const adjacency_matrix_t& adjacency_matrix)
        : individual(path, adjacency_matrix) {}

    ~WritableWithBasicIndividual() = default;
    WritableWithBasicIndividual(const WritableWithBasicIndividual&) = default;
    WritableWithBasicIndividual(WritableWithBasicIndividual&&) = default;
    WritableWithBasicIndividual& operator=(const WritableWithBasicIndividual&) = default;
    WritableWithBasicIndividual& operator=(WritableWithBasicIndividual&&) = default;

};
}