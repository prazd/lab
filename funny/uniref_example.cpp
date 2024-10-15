#include <iostream>

void f(int&& a) {
    std::cout << "i";
}

void f(float&& a) {
    std::cout << "f";
}

template <typename ...T>
void run(T&&... v) {
    (f(v), ...);
}

int main() {
    run(1.f, 2);
}