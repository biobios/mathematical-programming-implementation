#pragma once

#include <memory>
#include <vector>
#include <functional>

namespace mpi {
    template <typename T>
    class ObjectPool {
        class ObjectDeleter;
    public:
        using value_type = T;
        using pooled_unique_ptr = std::unique_ptr<T, ObjectDeleter>;
        using pooled_ptr = std::shared_ptr<T>;
        
        ObjectPool(size_t initial_size = 0)
            requires std::is_default_constructible_v<T>
            : factory([]() { return new T(); }), pool(std::make_shared<std::vector<std::unique_ptr<T>>>()) {
            pool->reserve(initial_size);
            for (size_t i = 0; i < initial_size; ++i) {
                pool->emplace_back(std::make_unique<T>());
            }
        }
       
        template <typename Factory>
            requires std::is_invocable_v<Factory> && std::is_same_v<T*, std::invoke_result_t<Factory>>
        ObjectPool(Factory&& factory, size_t initial_size = 0)
            : factory(std::forward<Factory>(factory)), pool(std::make_shared<std::vector<std::unique_ptr<T>>>()) {
            pool->reserve(initial_size);
            for (size_t i = 0; i < initial_size; ++i) {
                pool->emplace_back(factory());
            }
        }
        
        pooled_ptr acquire() {
            if (pool->empty()) {
                return std::shared_ptr<T>(factory(), ObjectDeleter(pool));
            } else {
                auto obj = pool->back().release();
                pool->pop_back();
                return std::shared_ptr<T>(obj, ObjectDeleter(pool));
            }
        }
        
        pooled_unique_ptr acquire_unique() {
            if (pool->empty()) {
                return pooled_unique_ptr(factory(), ObjectDeleter(pool));
            } else {
                auto obj = pool->back().release();
                pool->pop_back();
                return pooled_unique_ptr(obj, ObjectDeleter(pool));
            }
        }
        
        ObjectPool(const ObjectPool&) = delete; // コピーコンストラクタは削除
        ObjectPool& operator=(const ObjectPool&) = delete; // コピー代入演算子は削除
        
    private:
        class ObjectDeleter {
        public:
            void operator()(T* ptr) {
                // オブジェクトをプールに戻す
                if (auto valid_pool_ptr = pool.lock()) {
                    valid_pool_ptr->emplace_back(ptr);
                } else {
                    delete ptr; // プールがない場合は通常通り削除
                }
            }
        private:
            ObjectDeleter(std::weak_ptr<std::vector<std::unique_ptr<T>>> pool) : pool(pool) {}
            std::weak_ptr<std::vector<std::unique_ptr<T>>> pool;
            
            friend class ObjectPool;
        };

        std::function<T*()> factory;
        std::shared_ptr<std::vector<std::unique_ptr<T>>> pool;
    };
}