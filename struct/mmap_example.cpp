#include <iostream>
#include <sys/mman.h>

template <typename T, size_t sz>
struct Buffer {
    size_t offset;
    size_t capacity;
    T* memory;
    Buffer() : offset(0), capacity(sz),
    memory(static_cast<T*>(mmap(nullptr, sizeof(T) * sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0))) {
        if (memory == MAP_FAILED) throw std::runtime_error("mmap failed");
    }
    T* allocate(size_t size) {
        if (offset + size > capacity) throw std::bad_alloc();
        auto res = memory + offset;
        offset += size;
        return res;
    }
    void reset(){offset = 0;}
    ~Buffer(){    
        if (memory != MAP_FAILED) {
            munmap(memory, sizeof(T) * capacity);
        };
    }
};

template <std::bidirectional_iterator Iter>
void printr(Iter it, Iter ite) {
    for (;it != ite; --it)
        std::cout << (void*)it << " " << (unsigned int)(*it) << "\n";
}

int main() {
    Buffer<int, 1024 * 1024> buf;
    auto arr = buf.allocate(30);
    std::generate(arr, arr + 30, []{return 10;});
    printr(arr + 29, std::prev(arr));
    std::cout << "--------------------------------\n";
    auto arr2 = buf.allocate(10);
    std::generate(arr2, arr2 + 10, []{return 15;});
    printr(arr2 + 9, std::prev(arr2));
}