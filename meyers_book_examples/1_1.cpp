#include <iostream>

template <typename T, size_t N>
constexpr size_t array_size(T(&)[N]) noexcept {
    return N;
}

template <typename T>
void f(std::initializer_list<T> x){}




int main() {
    int a[21]{};
    int b[array_size(a)]{};
    static_assert(sizeof(a)/sizeof(a[0]) == sizeof(b)/sizeof(b[0]));
    f({1,2,3});
    ///

    std::vector<int> vec;

    // auto в возвращаемом типе функции или параметре лямбда-выражения влечет применение вывода типа шаблона, а не вывода типа auto.
    auto update_vec = [&vec](const auto& new_value){vec = new_value;};
    // update_vec({1,2,3,4}); !!! error
    
}