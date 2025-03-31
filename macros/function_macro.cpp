#include <iostream>

void func() {
    std::cout << "Result:" << __FUNCTION__ << std::endl;
    std::cout << "Result:" << __func__ << std::endl;
}

int main() {
    func();
}