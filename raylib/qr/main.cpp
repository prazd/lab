#include <iostream>
#include "raylib.h"
#include <bitset>

constexpr size_t DISPLAY_WIDTH = 300;
constexpr size_t DISPLAY_HEIGHT = 300;
constexpr size_t FIELD_WIDTH = 21;
constexpr size_t FIELD_HEIGHT = 21;

std::array<Color, FIELD_WIDTH * FIELD_HEIGHT> field{};

enum QRMode {
    Numeric = 0b0001,
    Alphanumeric = 0b0010,
    Byte = 0b0100,
    Kanji = 0b1000
};

enum ECC {
    M = 0b00, // 16 bytes
    L = 0b01, // 19 bytes
    H = 0b10, // 9 bytes
    Q = 0b11  // 13 bytes  
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

void draw_finder_7x7(size_t x, size_t y, auto& arr){
    static constexpr std::array<std::string_view, 7> finder_sv{
        "1111111",
        "1000001",
        "1011101",
        "1011101",
        "1011101",
        "1000001",
        "1111111",
    };
    for (int i = 0; i < 7; ++i){
        for (int j = 0; j < 7; ++j)
            arr[((y + i) * FIELD_WIDTH) + x + j] = finder_sv[i][j] == '1' ? BLACK : WHITE;
    }
}

void draw_timing_patterns(auto& arr){
    for (int i = 0; i < 5;++i){
        arr[(6 * FIELD_WIDTH) + i + 8] = !(i & 1) ? BLACK : WHITE;
        arr[((i + 8) * FIELD_WIDTH) + 6] = !(i & 1) ? BLACK : WHITE;
    }
}

void draw_format_bits(auto& arr){
    // mask0
    unsigned long val = 0b000 | (L << 3);
    // bch correction
    val <<= 10;
    auto get_h_b = [](unsigned long val, int sz){
        while(sz--){
            if ((val >> sz) & 1) break;
        }
        return sz;
    };
    auto gen = 0x537;
    auto shift = get_h_b(val, 15);
    while(shift >= 10){
        auto diff = shift - 10;
        val ^= gen << diff;
        shift = get_h_b(val, 15);
    }
    val ^= 0x5412;
    // 
    for (int i = 0; i < 8; ++i){
        arr[(i * FIELD_WIDTH) + 8] = (val >> i) & 1 ? BLACK : WHITE;
        arr[(8 * FIELD_WIDTH) + 20 - i] = (val >> i) & 1 ? BLACK : WHITE;
    }

    for (int i = 0; i < 7; ++i) {
        arr[(8 * FIELD_WIDTH) + i] = ((val >> 8) >> i) & 1 ? BLACK : WHITE;
        arr[((20 - i) * FIELD_WIDTH) + 8] = ((val >> 8) >> i) & 1 ? BLACK : WHITE;
    }
}

void draw_data(auto& arr, const auto& data){
    // todo
}


int main(){
    InitWindow(DISPLAY_WIDTH,DISPLAY_HEIGHT,"QR");
    auto img = GenImageColor(21, 21, WHITE);
    auto texture = LoadTextureFromImage(img);
    UnloadImage(img);

    auto r = gen_bitstream_bytemode(1, "Hello");
    auto gfr = add_gf_to_bitstream(r);
    // for (int i = 0; i < 208; ++i){
    //     std::cout << (int)((gfr[i/8] >> (i % 8)) & 1);
    // }
    // std::cout << std::endl;
    draw_finder_7x7(0,0,field);
    draw_finder_7x7(0,14,field);
    draw_finder_7x7(14,0,field);
    draw_timing_patterns(field);
    field[(8 * FIELD_WIDTH) + 13] = BLACK; // Dark module
    draw_format_bits(field);
    UpdateTexture(texture, field.data());

    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(WHITE);
        DrawTexturePro(texture, {0, 0, FIELD_WIDTH, FIELD_HEIGHT}, {0,0, DISPLAY_WIDTH, DISPLAY_HEIGHT},{0,0}, 0, WHITE);
        EndDrawing();
    }
    CloseWindow();
}
