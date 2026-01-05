#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <climits>

constexpr size_t HEIGHT = 200;
constexpr size_t WIDTH = 200;
constexpr char header_ascii[] = "P1\n200 200\n";
constexpr char header_bin[] = "P4\n200 200\n";

template <const char* str>
consteval size_t count_syms(){
    size_t res{};
    auto ptr = str;
    while(*(ptr++)) ++res;
    return res;
}

template <size_t height, size_t width>
consteval auto generate_ascii_pbm(){
    std::array<char, count_syms<header_ascii>() + height * width + height> res = {};
    auto ptr = header_ascii;
    size_t current = 0;
    while(current != count_syms<header_ascii>())
        *(res.data() + current++) = *ptr++;

    for (int i = 0; i < height; ++i){
         for (int j = 0; j < width; ++j)
             *(res.data() + (current++)) = ((i % 20) < 10 ? (((j % 20) >= 10)) : (((j % 20) < 10))) + '0';
          *(res.data() + (current++)) = '\n';
      }
      return res;
}

template <size_t height, size_t width>
consteval auto generate_bin_pbm(){
    constexpr size_t sz = (height * width)/CHAR_BIT;
    std::array<char, count_syms<header_bin>() + sz> res = {};
    auto ptr = header_bin;
    size_t current = 0;
    while(current != count_syms<header_bin>())
        *(res.data() + current++) = *ptr++;

    for (int i = 0; i < height * width; ++i) 
        *(res.data() + current + (i/CHAR_BIT)) |= 
            ((((i/width) % 20) < 10 ? ((((i%width) % 20) >= 10)) : ((((i%width) % 20) < 10))) << (7-i % CHAR_BIT));
    return res;
}


int main(){
    constexpr auto res_ascii = generate_ascii_pbm<HEIGHT, WIDTH>(); 
    constexpr auto res_bin = generate_bin_pbm<HEIGHT, WIDTH>(); 
    
    auto f1 = std::ofstream("res_ascii.pbm");
    f1 << res_ascii.data();
    f1.close();

    auto f2 = std::ofstream("res_bin.pbm", std::ios::binary);
    f2.write(res_bin.data(), res_bin.size());
    f2.close();
}