#include <iostream>
#include <fstream>
#include <sstream>
#include <array>

constexpr size_t HEIGHT = 200;
constexpr size_t WIDTH = 200;
constexpr char header[] = "P1\n200 200\n";

template <const char* str>
consteval size_t count_syms(){
    size_t res{};
    auto ptr = str;
    while(*(ptr++)) ++res;
    return res;
}

template <const char* header, size_t height, size_t width>
consteval auto generate_ascii_pbm(){
    std::array<char, count_syms<header>() + height * width + height> res = {};
    auto ptr = header;
    for(int i = 0; i < count_syms<header>(); ++i)
         *(res.data() + i) = *ptr++;
    size_t current = 11;
    for (int i = 0; i < HEIGHT; ++i){
         for (int j = 0; j < WIDTH; ++j)
             *(res.data() + (current++)) = ((i % 20) < 10 ? (((j % 20) >= 10)) : (((j % 20) < 10))) + '0';
          *(res.data() + (current++)) = '\n';
      }
      return res;
}


int main(){
    constexpr auto res = generate_ascii_pbm<header, HEIGHT, WIDTH>(); 
    auto s = std::ofstream("res.pbm");
    s << res.data();
    s.close();
}
