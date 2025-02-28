#include <iostream>
#include <cassert>

int main() {
    try {
        auto ptr = new long[100000000000000000];
        delete[] ptr;
    } catch (std::bad_alloc& e) {}
    auto ptr = new (std::nothrow) long[100000000000000000];
    assert(ptr == nullptr);
}