#pragma once
#include <vector>

template <typename T> class ObjectPool {
private:
    std::vector<T> pool;
    std::vector<T*> freeList;

public:
    ObjectPool(size_t maxSize) {
        pool.reserve(maxSize);
        freeList.reserve(maxSize);

        for (size_t i = 0; i < maxSize, ++i) {
            freeList.push_back(&pool[i]);
        }
    }

    T* acquire() {

        T* ptr = freeList.back();
        freeList.pop_back();

        return ptr;
    }

    void release(T* ptr) { freeList.push_back(ptr); }
};
