#include <iostream>
#include <meta>
#include <string_view>
#include <typeinfo>

struct S {
    unsigned id{};
    std::string_view name{};
};

// GCC 16.2
// --std=c++26 -O2 -freflection -fsanitize=address -fno-omit-frame-pointer 
template <typename T>
requires(std::is_class_v<T>)
void print(const T& val){
    constexpr std::meta::info struct_meta = ^^T;
    constexpr auto ctx = std::meta::access_context::current();
    template for (constexpr std::meta::info member : std::define_static_array(std::meta::nonstatic_data_members_of(struct_meta, ctx))){
        constexpr std::string_view sv = std::meta::identifier_of(member);
        std::cout << sv << ":" << val.[:member:] << '\n';
    }
    std::cout << '\n';
}


enum E {A = 0xa, B = 0xb, C = 0xc};

template <typename T>
requires(std::is_enum_v<E>)
void print(){
    template for (constexpr std::meta::info member : std::define_static_array(std::meta::enumerators_of(^^T))){
        std::cout << std::meta::identifier_of(member) << ' ' << [:member:] << '\n';
    }
    std::cout << '\n';
}

template <typename E>
requires(std::is_enum_v<E>)
consteval std::string_view to_sv(E val){
    template for (constexpr std::meta::info member : std::define_static_array(std::meta::enumerators_of(^^E))){
        if ([:member:] == val) return std::meta::identifier_of(member);
    }
    return "";
}

int main() {
    S s{256, "Alex"};
    print(s);

    print<E>();
    std::cout << to_sv<E>(A) << '\n';
    std::cout << to_sv<E>(B) << '\n';
    std::cout << to_sv<E>(C) << '\n';
}