#include <iostream>
#include <numeric>
#include <cassert>
#include <unistd.h>
#include <sys/types.h> 
#include <cstring>
#include <utility>
#include <cmath>

const void* BRK_START = sbrk(0);

template <typename T>
T* brk_alloc(size_t n) {
    return static_cast<T*>(sbrk(n));
}
void brk_free(void* address) {
    brk(address);
}
void brk_free() {
    brk(BRK_START);
}

class SimpleBrkAllocator {
    void* start = sbrk(0);
    unsigned char* begin = nullptr;
    unsigned char* current = nullptr;
    size_t page_sz{};
public:
    SimpleBrkAllocator(size_t start_page_sz = 4096): 
        begin((unsigned char*)sbrk(start_page_sz)), current(begin), page_sz(start_page_sz) {}

    template <typename T>
    T* allocate(size_t n) {
        if (current + (sizeof(T) * n) > begin + page_sz) {
            if (sizeof(T) * n > page_sz)
                page_sz = (page_sz * std::ceil((double)(sizeof(T) * n)/(double)page_sz));
            begin = static_cast<unsigned char*>(sbrk(page_sz));
            current = begin;
        }
        return static_cast<T*>(std::exchange(current, current + (sizeof(T) * n)));
    }

    ~SimpleBrkAllocator(){
        brk(start);
    }
};

int main() {
    std::array<int, 10> arr{};
    auto first = brk_alloc<int>(sizeof(arr));
    std::iota(arr.begin(), arr.end(), 0);
    memcpy(first, arr.data(), sizeof(arr));
    assert(std::equal(first, first + arr.size(), arr.begin()));
    brk_free();

    SimpleBrkAllocator sba;
    auto ptr = sba.allocate<unsigned char>(10);
    memset(ptr, 1, 10);
}