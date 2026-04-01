#include <meta>
#include <iostream>

// GCC trunk
template <typename T>
consteval auto to_members_arr(const T& t){
    return define_static_array(nonstatic_data_members_of(^^T, std::meta::access_context::current()));
}

consteval auto to_sv_arr(const auto& s) {
    static constexpr auto members = to_members_arr(s);
    std::array<std::string_view, std::size(members)> res;
    size_t i{};
    template for (const auto m: members)
        res[i++] = std::meta::identifier_of(m);
    return res;
}

struct S {
    int x{};
    int y{};
    int z{};
};

int main() {
    S s;
    constexpr auto res = to_sv_arr(s);
    static_assert(std::size(res) == 3);
    for (auto el: res) std::cout << el << ' ';
}