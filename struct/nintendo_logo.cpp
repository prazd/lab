#include <iostream>
#include <array>
#include <bitset>
#include <algorithm>
#include <utility>
#include <cstdio>

constexpr std::array<unsigned char, 48> logo = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

constexpr size_t sz = (48 * 8) + 8 + 1;

template <bool half>
consteval void copy_half(auto begin, auto end, auto& dst) {
    char tmp[4]{};
    for (auto it = begin, ite = end; it != ite; it+=2){
        auto cur = half ? *it : *it >> 4;
        for (int i = 0; i < 4; ++i){
            char c = cur >> ((4 - i - 1));
            tmp[i] = c & 1 ? '0' : '.';
        }
        std::copy(std::begin(tmp), std::end(tmp), std::exchange(dst, dst + 4));
        if (std::next(it) == ite) break;
    }
    *dst++ = '\n';
}

consteval auto decode_logo(const std::array<unsigned char, 48>& logo){
    std::array<char, sz> res{};
    auto ptr = res.data();
    copy_half<0>(logo.begin(), logo.begin() + 24, ptr);
    copy_half<1>(logo.begin(), logo.begin() + 24, ptr);
    copy_half<0>(logo.begin() + 1, logo.begin() + 25, ptr);
    copy_half<1>(logo.begin() + 1, logo.begin() + 25, ptr);
    copy_half<0>(logo.begin() + 24, logo.end(), ptr);
    copy_half<1>(logo.begin() + 24, logo.end(), ptr);
    copy_half<0>(logo.begin() + 25, logo.end(), ptr);
    copy_half<1>(logo.begin() + 25, logo.end(), ptr);
    res[res.size()-1] = '\0';
    return res;
}

int main() {
    constexpr auto res = decode_logo(logo);
    std::cout << res.data();
}