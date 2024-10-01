#include <iostream>
#include <list>

void* operator new(size_t n) {
    void* p = malloc(n);
    if (!p){throw std::bad_alloc{};}
    printf("Alloc:%p, size is %zu\n", p, n);
    return p;
} 

int main() {
    std::list<int> l;
    l.push_back(42);
}