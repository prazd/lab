#include <iostream>
#include <type_traits>

int main() {
    using Type = int[5];
    using Decayed = std::decay<Type>::type;
    std::cout << std::is_same<Decayed, int*>::value;  // true
}