#include <iostream>
#include <vector>
#include <malloc.h>

// linux
void print_memory_usage(){
    std::cout << "\n================================\n";
    auto info = mallinfo2();
    std::cout << "Memory usage:\n";
    std::cout << "Arena:" << info.arena / (1024) << "KB\n";
    std::cout << "Mmap fragments:" << info.hblkhd / (1024) << "KB\n";
    std::cout << "Usage mem:" << info.uordblks / (1024) << "KB\n";;
    std::cout << "Free mem:" << info.fordblks / (1024) << "KB\n";
    std::cout << "\n================================\n";
}

void* operator new(size_t sz){
    printf("Size:%lu", sz);
    return malloc(sz);
}

void operator delete(void* addr) {
    printf("Delete\n");
    free(addr);
}

int main() {
    {
        print_memory_usage();
        std::vector<int> x((1024 * 1024)/4,0);
        print_memory_usage();
    }
    print_memory_usage();
}