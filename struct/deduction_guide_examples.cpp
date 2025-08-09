#include <iostream>
#include <type_traits>

//  CTAD — Class Template Argument Deduction

// 1
template <typename ... Ts>
struct overloaded: Ts... {
    using Ts::operator()...;
};
template <typename ... Ts>
overloaded(Ts...) -> overloaded<Ts...>; // Deduction guide

// 2
template <typename T> 
struct S {
    T value;
    S(T value): value(value){}
};
S(const char*) -> S<std::string_view>;

int main() {
    auto f = overloaded([](int x){std::cout << "int";},[](float x){std::cout << "float";});
    
    static_assert(std::is_same_v<decltype(S("abc").value), std::string_view>);
}