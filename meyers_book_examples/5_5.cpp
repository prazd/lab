#include <iostream>

// Диспетчеризации дескрипторов (tag dispatch)

template <typename T>
void logic(T&& val, std::true_type) {
    std::cout << "Integral logic" << std::endl;
}

template <typename T>
void logic(T&& val, std::false_type) {
    std::cout << "Non integral logic" << std::endl;
}


template <typename T>
void run_logic(T&& val) {
    logic(std::forward<T>(val), std::is_integral<std::remove_reference_t<T>>());
}

int main() {
    const int& x = 200;
    run_logic(x);
}