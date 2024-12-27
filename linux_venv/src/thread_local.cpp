#include <iostream>

void test() {
    static int z = 0;
    thread_local int x = 0;
    ++x;
    auto d = malloc(4);
    thread_local int g = 0;
    thread_local int y = 0;
    thread_local int y1 = 0;
    thread_local int y2 = 0;
    std::cout << &x << "\n";
    std::cout << &z << "\n";
    std::cout << d << "\n";
    std::cout << &g << "\n";
    std::cout << &y << "\n";
    std::cout << &y1 << "\n";
    std::cout << &y2 << "\n";
    free(d);
}

int main() {
    test();
}