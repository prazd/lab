#include <iostream>

void print_trianle(uint64_t num) {
    std::string spaces(num-1, ' ');
    std::string current = ".";
    for (size_t i = 0; i < num; ++i) {
        std::cout << spaces << current  << "\n";
        spaces.pop_back();
        current += " .";
    }
}

uint64_t gauss_next(uint64_t n) {
    return (n * (n + 1)) / 2;
}

void print_gauss_num(uint64_t n) {
    std::cout << "Gauss num:" << gauss_next(n) << "\n";
    print_trianle(n);
}

int main() {
    // 1 - 127 view
    print_gauss_num(127);
}
