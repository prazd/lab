#include <iostream>

int main() {
    std::cout << std::is_convertible<int, double>::value;  // true
    std::cout << std::is_convertible<double, int>::value;  // true
}