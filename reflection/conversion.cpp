#include <iostream>
#include <array>
#include <meta>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <tuple>
#include <vector>

template <typename T>
void print_tuple(const T& val){
    constexpr auto static args = std::define_static_array(std::meta::template_arguments_of(^^T));
    size_t i = 0;
    template for (constexpr std::meta::info type : args)
        std::cout << i++ << ':' <<  std::get<typename [:type:]>(val) << '\n';
    std::cout << '\n';
}

template <typename T>
requires(std::is_class_v<T>)
void print_struct(const T& val){
    constexpr std::meta::info struct_meta = ^^T;
    constexpr auto ctx = std::meta::access_context::current();
    template for (constexpr std::meta::info member : std::define_static_array(std::meta::nonstatic_data_members_of(struct_meta, ctx))){
        constexpr std::string_view sv = std::meta::identifier_of(member);
        std::cout << sv << ":" << val.[:member:] << '\n';
    }
    std::cout << '\n';
}

struct S {
    int x = 10; unsigned y = 11; float z = 12.0f;
};

template <typename S>
requires(std::is_class_v<S>)
auto to_tuple(const S& s){
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


template <size_t N, size_t MAX_N = 3>
consteval auto gen_field_name(){
    std::array<char,1 + MAX_N + 1> arr{'f'};
    auto n = N;
    do {
        auto c = n%10;
        arr[1] ='0'+c;
        n=n/10;
        if (n){
            for (size_t i = 0; i < MAX_N; ++i)
                std::swap(arr[1 + MAX_N-i],arr[1 + MAX_N-i-1]);
        }
    } while(n);
    return std::define_static_string(arr);
}

template <typename Tuple>
auto to_struct(const Tuple& t){
    constexpr auto static args = std::define_static_array(std::meta::template_arguments_of(^^Tuple));
    return []<typename T, size_t ... I>(const T& t, std::index_sequence<I...>){
        struct S;
        consteval{
        std::vector<std::meta::info> vec;
        size_t z = 0;
            (vec.push_back(
                std::meta::data_member_spec(args[I], std::meta::data_member_options{.name = gen_field_name<I>()})
                ), ...);
            std::meta::define_aggregate(^^S, vec);
        }
        S s;
        static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::current()));
        ((s.[:members[I]:] = std::get<I>(t)),...);
        return s;

    }(t, std::make_index_sequence<args.size()>());
}

template <typename S1, typename S2>
bool eq_struct(const S1& lhs, const S2& rhs){
    static constexpr auto members_s1 = std::define_static_array(std::meta::nonstatic_data_members_of(^^S1, std::meta::access_context::current()));
    static constexpr auto members_s2 = std::define_static_array(std::meta::nonstatic_data_members_of(^^S2, std::meta::access_context::current()));
    if (members_s1.size() != members_s2.size()) return false;
    return []<typename T1, typename T2, size_t ... I>(const T1& t1, const T2& t2, std::index_sequence<I...>){
        return (((t1.[:members_s1[I]:] == t2.[:members_s2[I]:])) && ...);
    }(lhs, rhs, std::make_index_sequence<members_s1.size()>());
}

template <bool b>
auto substitute(){
    constexpr auto s = b ? std::meta::substitute(^^std::array,{^^int, std::meta::reflect_constant(10)})
    : std::meta::substitute(^^std::vector,{^^int});
    using T = [:s:];
    T t;
    if constexpr (std::is_same_v<std::vector<int>, T>)
        t.push_back(42);
    else
        t[0] = 42;
    return t[0];
}

int main(){
    S s;
    auto t = to_tuple(s);
    print_tuple(t);
    auto s2 = to_struct(t);
    print_struct(s2);
    auto s3 = to_struct(t);
    bool eq = eq_struct(s2, s3);
    std::cout << "S1==S2:" << (int)eq << '\n';
    std::cout << "substitude1 == substitute2:" << (substitute<true>() == substitute<false>()) << '\n';
}