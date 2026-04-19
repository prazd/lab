#include <iostream>
#include <array>
#include <functional>

template <typename Func, typename Seq, size_t ... I>
constexpr decltype(auto) unpack_and_call(Func&& f, Seq&& s, std::index_sequence<I...>){
    return std::invoke(std::forward<Func>(f), std::get<I>(std::forward<Seq>(s))...);
}

template <typename Func, typename Seq>
constexpr decltype(auto) apply(Func&& f, Seq&& s){
    auto seq = std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Seq>>>{};
    return unpack_and_call(std::forward<Func>(f), std::forward<Seq>(s), seq);
}

struct S {
    int val = 5;
    constexpr int foo(int a)const{return val + a;}
} constexpr s;

int main(){
    static_assert(apply(&S::foo, std::tuple<const S&, int>(s, 5)) == 10);
    static_assert(apply([](int a, int b, int c){return a + b + c;}, std::array{8,2,10}) == 20);
    static_assert(apply([](int a, int b){return a + b;}, std::tuple(15,15)) == 30);
    static_assert(apply([](int a, int b){return a + b;}, std::pair(25,15)) == 40);
    static_assert(apply([](int a){
        int b = 32;
        apply([](int& val){ val += 8;}, std::tuple<int&>(b));
        return a + b;
        }, std::tuple(10)) == 50);
}