#pragma once

#include <cstddef>
#include <vector>
#include <limits>
#include <stdexcept>

namespace mpi
{
    class LimitedRangeIntegerSet {
    public:
        using size_type = size_t;
        using iterator = typename std::vector<size_t>::const_iterator;
        using const_iterator = typename std::vector<size_t>::const_iterator;
        enum class InitSet {
            Empty,
            Universal
        };

        LimitedRangeIntegerSet(size_t max, InitSet init_set = InitSet::Universal, size_t min = 0) 
            : MAX(max), MIN(min)
        {
            switch (init_set) {
                case InitSet::Empty:
                    positions.resize(max - min + 1, std::numeric_limits<size_t>::max());
                    break;
                case InitSet::Universal:
                    elements.reserve(max - min + 1);
                    positions.reserve(max - min + 1);
                    for (size_t i = 0; i <= max - min; ++i) {
                        elements.push_back(i + min);
                        positions.push_back(i);
                    }
                    break;
                default:
                    throw std::invalid_argument("Invalid initialization set");
            }
        }
        
        void reset(InitSet init_set = InitSet::Universal)
        {
            switch (init_set) {
                case InitSet::Empty:
                    elements.clear();
                    positions.assign(MAX - MIN + 1, std::numeric_limits<size_t>::max());
                    break;
                case InitSet::Universal:
                    elements.clear();
                    elements.reserve(MAX - MIN + 1);
                    positions.clear();
                    for (size_t i = 0; i <= MAX - MIN; ++i) {
                        elements.push_back(i + MIN);
                        positions.push_back(i);
                    }
                    break;
                default:
                    throw std::invalid_argument("Invalid initialization set");
            }
        }
        
        bool contains(size_t value) const
        {
            if (value < MIN || value > MAX) {
                return false;
            }
            
            return positions[value - MIN] != std::numeric_limits<size_t>::max();
        }
        
        size_type erase(size_t value)
        {
            if (value < MIN || value > MAX) {
                return 0;
            }
            
            size_t pos = positions[value - MIN];
            if (pos == std::numeric_limits<size_t>::max()) {
                return 0; // 値が存在しない
            }
            
            size_t last_value = elements.back();
            elements[pos] = last_value;
            positions[last_value - MIN] = pos;
            elements.pop_back();
            positions[value - MIN] = std::numeric_limits<size_t>::max();
            
            return 1; // 成功
        }
        
        std::pair<iterator, bool> insert(size_t value)
        {
            if (value < MIN || value > MAX) {
                throw std::out_of_range("Value out of range");
            }
            
            size_t pos = positions[value - MIN];
            if (pos != std::numeric_limits<size_t>::max()) {
                return {elements.begin() + pos, false}; // 既に存在する
            }
            
            elements.push_back(value);
            pos = elements.size() - 1;
            positions[value - MIN] = pos;
            
            return {elements.begin() + pos, true}; // 新たに挿入された
        }
        
        iterator begin() const
        {
            return elements.cbegin();
        }
        
        const_iterator cbegin() const
        {
            return elements.cbegin();
        }

        iterator end() const
        {
            return elements.cend();
        }
        
        const_iterator cend() const
        {
            return elements.cend();
        }
        
        size_type size() const
        {
            return elements.size();
        }
        
    private:
        const size_t MAX;
        const size_t MIN;
        std::vector<size_t> elements;
        std::vector<size_t> positions;
    };
}