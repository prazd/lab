#include <iostream>
#include <vector>
#include <malloc.h>
#include <sstream>

// linux
enum class MemoryUnit {B, KB, MB};
constexpr long B = 1;
constexpr long KB = 1024;
constexpr long MB = 1024 * 1024;

template <typename T>
void Delete(T) = delete;

void print_memory_usage(MemoryUnit sz){
    std::pair<long, std::string_view> p;
    switch (sz) {
        case MemoryUnit::B:  {p.first = B;  p.second = "B";   break;}
        case MemoryUnit::KB: {p.first = KB; p.second = "KB";  break;}
        case MemoryUnit::MB: {p.first = MB; p.second = "MB";  break;}
    }
    std::cout << "\n================================\n";
    auto info = mallinfo2();
    std::cout << "Memory usage:\n";
    std::cout << "Arena:" << info.arena / p.first << p.second <<"\n";
    std::cout << "Mmap fragments:" << info.hblkhd / p.first << p.second <<"\n";
    std::cout << "Total allocated by user (uordblks):" << info.uordblks / p.first << p.second <<"\n";
    std::cout << "Total free space (fordblks): " << info.fordblks / p.first << p.second <<"\n";
    std::cout << "Fastbin free space (fsmblks): " << info.fsmblks / p.first << p.second << "\n";
    std::cout << "Ordinary free blocks (ordblks): " << info.ordblks << "\n";
    std::cout << "\n================================\n";
}

void* operator new(size_t sz){
    printf("\nAlloc:%lu\n", sz);
    return malloc(sz);
}

void operator delete(void* addr) {
    printf("\nFree\n");
    free(addr);
}

int main() {
    {
        print_memory_usage(MemoryUnit::KB);
        std::cout << "\nInit 1mb vector:\n";
        std::vector<int> x((1024 * 1024)/4,0);
        print_memory_usage(MemoryUnit::KB);
    }
    std::cout << "\nAfter vector free\n";
    print_memory_usage(MemoryUnit::KB);

    std::cout << "\n\nAlloc arr (10000 int):\n";
    auto ptr = new int[10000];
    print_memory_usage(MemoryUnit::KB);
    std::cout << "\n\nAfter free arr:\n";
    delete[] ptr;
    print_memory_usage(MemoryUnit::KB);
}