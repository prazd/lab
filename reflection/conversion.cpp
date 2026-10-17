#include <iostream>
#include <array>
#include <meta>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <tuple>

template <typename T>
void print(const T& val){
    constexpr auto static args = std::define_static_array(std::meta::template_arguments_of(^^T));
    size_t i = 0;
    template for (constexpr std::meta::info type : args)
        std::cout << i++ << ':' <<  std::get<typename [:type:]>(val) << '\n';
    std::cout << '\n';
}

struct S {
    int x = 10; unsigned y = 11; float z = 12.0f;
};

template <typename S>
requires(std::is_class_v<S>)
auto to_tuple(S& s){
    static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::current()));
    auto init_t = []<size_t ... I>(std::index_sequence<I...>){
        return std::tuple<typename [:std::meta::type_of(members[I]):]...>{};
    };
    auto fill_t = []<typename Struct, typename Tuple,  size_t ... I>(Struct& s, Tuple& t, std::index_sequence<I...>){
        ((std::get<I>(t) = s.[:members[I]:]),...);
    };
    auto seq = std::make_index_sequence<members.size()>();
    auto res = init_t(seq);
    fill_t(s,res,seq);
    return res;    
}

int main(){
    S s;
    auto t = to_tuple(s);
    print(t);
}