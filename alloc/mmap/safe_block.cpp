#include <iostream>
#include <sys/mman.h>
#include <cassert>
#include <optional>
#include <vector>

template <typename Container>
struct SafeBlock {
    using ElementType = typename Container::value_type;
    size_t count{};
    ElementType* ptr{};

    SafeBlock(const SafeBlock&) = delete;
    SafeBlock& operator=(const SafeBlock&) = delete;
    
    SafeBlock(const Container& container): count(container.size()), ptr((ElementType*)mmap(nullptr, count * sizeof(ElementType), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) {
        if (ptr == MAP_FAILED) {
            perror("mmap");
            throw std::bad_alloc();
        }
        std::copy(std::begin(container), std::end(container), ptr);
        
        if (mprotect(ptr, count * sizeof(ElementType), PROT_READ) != 0) {
            perror("mprotect");
            throw std::bad_alloc();
        }
    }

    const ElementType& operator[](size_t index) const {
        return *(ptr + index);
    }

    auto cbegin() const {
        return ptr;
    }

    auto cend() const {
        return ptr + count;
    }

    ~SafeBlock() {
        munmap(ptr, sizeof(ElementType) * count);
    }
};

int main() {
    SafeBlock sb(std::vector<int>{10,20,30});
    assert(sb[0] == 10);
    assert(sb[1] == 20);
    assert(sb[2] == 30);

    assert(*(sb.cbegin())     == 10);
    assert(*(sb.cbegin() + 1) == 20);
    assert(*(sb.cbegin() + 2) == 30);
}