#include <iostream>
#include "raylib.h"
#include <bitset>

constexpr size_t WIDTH = 300;
constexpr size_t HEIGHT = 300;


enum QRMode {
    Numeric = 0b0001,
    Alphanumeric = 0b0010,
    Byte = 0b0100,
    Kanji = 0b1000
};
// ByteMode 
constexpr unsigned char first_add_byte = 0b11101100;
constexpr unsigned char second_add_byte = 0b00010001;
constexpr unsigned char reed_solomon_coeffs[7] = {
    87,229,146,149,238,102,21
};

struct GF256 {
    unsigned char exp_t[256]{};
    unsigned char log_t[512]{};
    consteval GF256(){
        unsigned char x = 1;
        for (int i = 0; i < 255; ++i){ 
            exp_t[i] = x; 
            log_t[x] = i; 
            x = x * 2; 
            if (x & 0x100) x ^= 0x11d;
        }
    }
    constexpr unsigned char mul(unsigned char a, unsigned char b){
        if (a == 0 || b == 0) return 0;
        return exp_t[log_t[a] + log_t[b]];
    }
} gf;

unsigned char length_bitsz(QRMode mode){
    switch(mode){
        case Numeric: return 10;
        case Alphanumeric: return 9;
        default : return 8;
    }
}

auto gen_bitstream_bytemode(unsigned char sz, std::string_view str){
    std::array<unsigned char, 19> res{}; // 152 bit
    size_t pos = 0;
    auto write = [](auto& arr, auto& pos_value, unsigned char value, int sz){
        while(sz--){
            auto cur_byte_pos = pos_value / 8;
            auto cur_bit_pos = pos_value % 8;
            arr[cur_byte_pos] |= (value & 1) << cur_bit_pos;
            value >>= 1;
            ++pos_value;
        }
    };
    write(res, pos, Byte, 4);
    write(res, pos, str.size(), length_bitsz(Byte)); // length
    for (int i = 0, e = str.size(); i != e; ++i)
        write(res, pos, str[i], 8);
    // add terminator
    if (pos <= 148)
        write(res, pos, 0, 4);
    // add padding
    if (pos % 8 && pos != 152)
        write(res, pos, 0, 8-(pos % 8));
    if (pos != 152){
        bool first = true;
        while(pos != 152){
            if (first) write(res, pos, first_add_byte,  8);
            else       write(res, pos, second_add_byte, 8);
            first = !first;
        }
    }
    return res;
}

auto add_gf_to_bitstream(auto& bitstream) {
    std::array<unsigned char, 26> res{};
    std::copy(std::begin(bitstream), std::end(bitstream),res.begin());
    for (int i = 0; i < 19; ++i){
        auto factor = res[i];
        if (!factor) continue;
        for (int j = 0; j < 7; ++j)
            res[i + j + 1] ^= gf.mul(reed_solomon_coeffs[j], factor);
    }
    return res;
}


int main(){
    InitWindow(WIDTH,HEIGHT,"QR");
    auto r = gen_bitstream_bytemode(1, "Hello");
    auto gfr = add_gf_to_bitstream(r);
    for (int i = 0; i < 208; ++i){
        std::cout << (int)((gfr[i/8] >> (i % 8)) & 1);
    }
    std::cout << std::endl;
    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(WHITE);
        EndDrawing();
    }
    CloseWindow();
}
