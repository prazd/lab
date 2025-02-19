#include <iostream>
#include <cassert>

struct alignas(64) S {
    long x;
    long y;
};

template <typename T, size_t cap>
struct Data {
    alignas(sizeof(T)) char buffer[cap * sizeof(T)];
    size_t sz{};
    void push(T value){
        if (sz != cap)
            ((T*)buffer)[sz++] = value;
    }
    void print() {
        for(auto it = (unsigned char*)(((T*)buffer)+sz)-1, ite = (unsigned char*)(((T*)buffer))-1; it != ite; --it)
            std::cout << (void*)it << " " << (unsigned int)(*it) << "\n";
    }
};

int main() {
    Data<S, 10> d;
    assert((unsigned long)(&d.buffer) % sizeof(S) == 0);
    d.push({10,20});
    d.push({30,40});
    d.print();
}