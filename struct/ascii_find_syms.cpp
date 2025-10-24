#include <iostream>
#include <numeric>

constexpr std::array<char, 128> ascii_syms = []{
    std::array<char, 128> res;
    std::iota(res.begin(), res.end(), 0);
    return res;
}();

consteval auto find_syms(auto func) {
    std::array<char, std::count_if(ascii_syms.begin(), ascii_syms.end(), func)> res;
    for (int i = 0; auto el: ascii_syms)
        if (func(el)) res[i++] = el;
    return res;
}

int main() {
    constexpr auto res = find_syms([](auto el){
        return  (el << 5) & 64;
    });
    for (auto el: res)
        std::cout << (unsigned)el << ":" << el << '\n';
}