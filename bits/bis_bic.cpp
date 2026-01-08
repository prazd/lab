#include <iostream>

// bis - bit set
template <std::unsigned_integral T>
consteval T bis(T v, T m) {
    return v | m;
}

// bic - bit clear
template <std::unsigned_integral T>
consteval T bic(T v, T m) {
    return v & ~m;
}

template <std::unsigned_integral T>
consteval T t_or(T x, T y){
    return bis(x, y);
}

template <std::unsigned_integral T>
consteval T t_xor(T x, T y) {
    return bis(bic(x,y), bic(y,x));
}

template <std::unsigned_integral T>
consteval T t_and(T x, T y) {
    return bic(x, ~y);
}

template <std::unsigned_integral T>
consteval bool equal(T a, T b){
    return !(t_xor(a, b));
}

int main(){
    static_assert(t_or(8u,2u) == 10u);
    static_assert(t_xor(8u,9u) == 1u);
    static_assert(t_and(8u,9u) == 8u);
    static_assert(!equal(10u,20u));
    static_assert(equal(20u,20u));
}