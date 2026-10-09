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
void print_fields(const T& val){
    constexpr std::meta::info struct_meta = ^^T;
    constexpr auto ctx = std::meta::access_context::current();
    template for (constexpr std::meta::info member : std::define_static_array(std::meta::nonstatic_data_members_of(struct_meta, ctx))){
        constexpr std::string_view sv = std::meta::identifier_of(member);
        std::cout << sv << ":" << val.[:member:] << '\n';
    }
    std::cout << '\n';
}

int main() {
    S s{256, "Alex"};
    print_fields(s);
}