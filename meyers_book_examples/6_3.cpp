#include <iostream>

template <typename T>
void print(T&& val) {
    std::cout << val << std::endl;
}

int main() {
    // Используйте для параметров аutо&& при их прямой передаче с помощью std::forward ключевое слово decltype.
    auto l = [](auto&& val){print(std::forward<decltype(val)>(val));};
    l(32);
}