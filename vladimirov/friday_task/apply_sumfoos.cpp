#include <tuple>
#include <cassert>
#include <array>
#include <numeric>

template <typename T> constexpr int foo() { return 1; } 
template <> constexpr int foo<int>() { return 2; }
template <> constexpr int foo<float>() { return 4; }

template <typename ...T>
constexpr int sumfoos() { return (foo<T>() + ...); }

template <typename Tuple> 
constexpr auto apply_sumfoos(){
    return []<size_t ... Is>(std::index_sequence<Is...>){
        return sumfoos<typename std::tuple_element<Is, Tuple>::type...>();
    }(std::make_index_sequence<std::tuple_size<Tuple>::value>{});
}

int main() {
    static_assert(apply_sumfoos<decltype(std::tuple<int, float, double>{})>() == 7);
}