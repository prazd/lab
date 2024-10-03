#include <iostream>

void func() {
    std::cout << "Result:" << __FUNCTION__ << std::endl;
}

int main() {
    func();
}