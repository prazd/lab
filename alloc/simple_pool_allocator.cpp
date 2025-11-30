#include <iostream>
#include <deque>
#include <utility>
#include <cassert>
#include <cmath>

template <size_t buffer_sz = 1024, size_t chunk_sz = 8>
struct SimplePoolAllocator{
    void* base{};
    std::deque<void*> valid_addresses{};
    SimplePoolAllocator(): base(malloc(buffer_sz)){
        auto ptr = (unsigned char*)base;
        size_t offset = 0;
        for (int i = 0; i < buffer_sz/chunk_sz; ++i)
            valid_addresses.push_back(ptr + std::exchange(offset, offset + chunk_sz));
        
    }
    std::optional<decltype(valid_addresses.begin())> find_space(size_t chunk_n){
        size_t n = 0;
        bool ok = true;
        for (int i = 0; i < valid_addresses.size(); ++i){
            for (int j = 0; j < chunk_n-1; ++j) {
                if (i + j >= valid_addresses.size()) 
                    ok = false;
                else if ((unsigned char*)(valid_addresses[i]) + (chunk_sz * j) != (unsigned char*)(valid_addresses[i + j]))
                    ok = false;
            }
            if (ok) 
                return valid_addresses.begin() + i;
            ok = true;
        }
        return {};
    }
    void* allocate(size_t n) {
        auto chunks_n = std::ceil(double(n)/double(chunk_sz));
        auto space_res = find_space(chunks_n);
        if (!space_res.has_value())
            throw std::bad_alloc();
        auto address = *space_res.value();
        valid_addresses.erase(space_res.value(), space_res.value() + chunks_n);
        return address;
    }
    void deallocate(void* addr, size_t n) {
        auto chunks_n = std::ceil(double(n)/double(chunk_sz));
        auto ptr = (unsigned char*)(addr);
        size_t offset = 0;
        for (int i = 0; i < chunks_n; ++i)
            valid_addresses.push_back(ptr + std::exchange(offset, offset + chunk_sz));
        std::sort(valid_addresses.begin(), valid_addresses.end());
    }
    size_t size() const {
        return buffer_sz;
    }
    size_t chunk_size() const {
        return chunk_sz;
    }
    ~SimplePoolAllocator() {
        free(base);
    }
};

int main() {
    SimplePoolAllocator spa;
    auto addr = spa.allocate(10);
    assert((unsigned char*)spa.base + 16 == (unsigned char*)(spa.valid_addresses.front()));
    spa.deallocate(addr, 10);
    assert((unsigned char*)spa.base == (unsigned char*)(*(spa.valid_addresses.begin())));
    assert(((unsigned char*)spa.base) + spa.chunk_size() == (unsigned char*)(*(spa.valid_addresses.begin() + 1)));
}