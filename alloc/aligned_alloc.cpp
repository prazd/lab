#include <iostream>
int main() {
    constexpr size_t alignment = 32;
    constexpr size_t sz = 1024;
    auto ptr = std::aligned_alloc(alignment, sz);
    std::cout << "Addr:" << ptr << '\n';
    free(ptr);
}