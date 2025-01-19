#include <iostream>

int main() {
    union {
        long x;
        float y;
        char z[3];
    };
    x = 10;
    std::cout << x << "\n";
    y = 0.1f;
    std::cout << y << "\n";
    z[0] = 120;
    std::cout << (int)z[0] << "\n";
}