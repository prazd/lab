#include <iostream>
#include <type_traits>

int main() {
    using Type = int&;
    using NoRef = std::remove_reference<Type>::type;
    std::cout << std::boolalpha << std::is_same<NoRef, int>::value;  // true
}