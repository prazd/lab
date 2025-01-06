#include <iostream>
#include <memory_resource>


void* operator new(size_t sz) {
    std::cout << "Alloc:" << sz << "\n";
    return malloc(sz);
}

template <typename T>
void print_bytes(const T& val) {
    auto ptr = (std::byte*)&val;
    for(auto it = ptr+(sizeof(val)-1), ite = ptr-1; it != ite; --it)
        std::cout << (void*)it << " " << (int)(*it) << "\n";
}

int main() {
    std::byte stackBuffer[48];
    std::pmr::monotonic_buffer_resource mb(stackBuffer, sizeof(stackBuffer));
    std::pmr::vector<int> vec{{1,2,3,4,5,6}, &mb};
    std::pmr::vector<int> vec2{{7,8,9,10,11,12}, &mb};
    print_bytes(stackBuffer);
}