#include <iostream>
#include <vector>
#include <cstring>
#include <memory>

template <typename T, size_t sz>
struct StackAllocator {
    template <typename U>
    struct rebind {
        using other = StackAllocator<U, sz>;
    };

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    char buffer[sz];
    size_t last = 0;

    StackAllocator() = default;
    StackAllocator(StackAllocator&&) = default;
    StackAllocator(const StackAllocator&) = default;
    StackAllocator& operator=(StackAllocator&&) = default;
    StackAllocator& operator=(const StackAllocator&) = default;

    size_type max_size() const noexcept {
        return sz / sizeof(T);
    }

    T* allocate(size_t n) {
        size_t required_size = n * sizeof(T);
        if (last + required_size > sz) {
            throw std::bad_alloc();
        }

        void* res = buffer + last;

        if (reinterpret_cast<uintptr_t>(res) % alignof(T) != 0) {
            throw std::bad_alloc();
        }

        last += required_size;
        return static_cast<T*>(res);
    }

    void deallocate(T* p, size_t n) {}

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        ::new ((void*)p) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p) {
        p->~U();
    }

    ~StackAllocator() = default;
};

int main() {
    std::vector<int, StackAllocator<int, 200>> x{40,41};
    x.push_back(42);
    x.push_back(43);

    for (auto v : x) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    return 0;
}
