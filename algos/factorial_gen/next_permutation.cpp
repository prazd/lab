#include <iostream>
#include <array>
#include <algorithm>

template <typename It>
void print(It it, It ite){
    for(;it!=ite;++it)
        std::cout << *it << " ";
    std::cout << "\n";
}


void* operator new(size_t sz){
    std::cout << "Alloc:" << sz << "\n";
    return malloc(sz);
}

int main() {
    std::array<int, 5> vec{1,2,3,4,5};
    print(vec.begin(), vec.end());
    while(std::next_permutation(vec.begin(), vec.end()))
        print(vec.begin(), vec.end());
}