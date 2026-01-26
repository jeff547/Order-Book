#pragma once
#include <new>
#include <utility>

template <typename T>
class ObjectPool {
private:
    union Slot {
        T obj;
        Slot* next;

        Slot() {}
        ~Slot() {}
    };

    Slot* pool;
    Slot* freeList = nullptr;
    size_t capacity;

public:
    ObjectPool(size_t n)
        : capacity(n) {
        pool = static_cast<Slot*>(::operator new[](capacity * sizeof(Slot)));
        reset();
    }

    ~ObjectPool() { ::operator delete[](pool); }

    template <typename... Args>
    T* acquire(Args&&... args) {
        if (freeList == nullptr)
            return nullptr;

        Slot* slot = freeList;
        freeList = slot->next;

        return new (&slot->obj) T(std::forward<Args>(args)...);
    }

    void release(T* obj) {
        if (!obj)
            return;

        obj->~T();

        Slot* slot = reinterpret_cast<Slot*>(obj);
        slot->next = freeList;
        freeList = slot;
    }

    void reset() {
        for (size_t i = 0; i < capacity - 1; i++) {
            pool[i].next = &(pool[i + 1]);
        }
        pool[capacity - 1].next = nullptr;
        freeList = &(pool[0]);
    }
};
