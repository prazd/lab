#include <iostream>

template <typename T>
void foo(typename std::type_identity<T>::type v){}

template <typename T>
auto bound(T value, typename std::type_identity<T>::type min, typename std::type_identity<T>::type max) {
    return std::clamp(value, min, max);
}

int main() {
    bound(25.4, 10, 30); // ok
    foo<int>(10); // ok
    // foo(10); // compile error
}