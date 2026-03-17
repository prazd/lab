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
    0x01, 0x11, 0xEF, 0x36, 0xD4, 0x78, 0xCA
};

struct GF256 {
    unsigned char exp_t[256]{};
    unsigned char log_t[256]{};
    consteval GF256(){
        unsigned char x = 1;
        log_t[0] = 0xff;
        exp_t[0] = 1;
        for (int i = 1; i < 255; ++i){ 
            x = ((x << 1) ^ ((x & 0x80) ? 0x11d : 0));
            exp_t[i] = x; 
            log_t[x] = i; 
        }
    }
    constexpr unsigned char mul(unsigned char a, unsigned char b){
        if (a == 0 || b == 0) return 0;
        return exp_t[(log_t[a] + log_t[b]) & 0xff];
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
        auto s_sz = sz;
        while(sz--){
            auto cur_byte_pos = pos_value / 8;
            auto cur_bit_pos = pos_value % 8;
            arr[cur_byte_pos] |= ((value >> (s_sz - 1)) & 1) << (7 - cur_bit_pos);
            value <<= 1;
            ++pos_value;
        }
    };
    write(res, pos, Byte, 4);
    write(res, pos, str.size(), 8);
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
    std::copy(std::begin(bitstream), std::end(bitstream), res.begin() + 7);
    for (int i = 7; i < 26; ++i){
        auto factor = res[i];
        if (!factor) continue;
        for (int j = 0; j < 7; ++j)
            res[(i-7)+j] ^= gf.mul(reed_solomon_coeffs[j], factor);
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

void use_simple_mask(auto& arr){
    for (int y = 0; y < 21; ++y){
        for (int x = 0; x < 21; ++x){
            if (!((x == 8 && y == 13) || 
            (y < 7 && ((x >= 0 && x <= 8) || (x >= 13 && x <= 20))) ||
            (y >= 14 && (x >= 0 && x <= 8)) || 
            (y == 6 && (x >= 8 && x <= 12)) || 
            ((y >= 8 && y <= 12) && x == 6) ||
            (y == 8 && ((x >= 0 && x <= 5) || (x >= 7 && x <= 8) || (x >= 13 && x <= 20))) ||
             (x == 8 && ((y >= 0 && y <= 5) || (y >= 7 && y <= 8) || (y >= 13 && y <= 20)))
            )){
                if ((x + y) % 2 == 0){
                    if (arr[(y * FIELD_WIDTH) + x].r == 255)
                        arr[(y * FIELD_WIDTH) + x] = BLACK;
                    else
                        arr[(y * FIELD_WIDTH) + x] = WHITE;
                }
        } 
        }
    }
}

void draw_data(auto& arr, const auto& data){
    size_t current = 0;
    int x = 20;
    int y = 20;
    bool up = true;
    bool right = true;
    while(current != 208){
        if (!((x == 8 && y == 13) || 
            (y < 7 && ((x >= 0 && x <= 8) || (x >= 13 && x <= 20))) ||
            (y >= 14 && (x >= 0 && x <= 8)) || 
            (y == 6 && (x >= 8 && x <= 12)) || 
            ((y >= 8 && y <= 12) && x == 6) ||
            (y == 8 && ((x >= 0 && x <= 5) || (x >= 7 && x <= 8) || (x >= 13 && x <= 20))) ||
            (x == 8 && ((y >= 0 && y <= 5) || (y >= 7 && y <= 8) || (y >= 13 && y <= 20)))
        ) 
        ) {
            arr[(y * FIELD_WIDTH) + x] = ((data[current/8] >> (7-((current++)%8))) & 1) ? BLACK : WHITE;
        }
        if (up && y == 0) {
            up = false;
            x-=2;
            right = true;
        } else if (!up && y == 20){
            up = true;
            x-=2;
            right = true;
        } else if (right) {
            x-=1;
            if (x == 6) --x;
            right = false;
        } else {
            x += 1;
            if (x==6) ++x;
            right = true;
            y += up ? -1 : 1;
        }
    }
}


int main(){
    InitWindow(DISPLAY_WIDTH,DISPLAY_HEIGHT,"QR");
    auto img = GenImageColor(21, 21, WHITE);
    auto texture = LoadTextureFromImage(img);
    UnloadImage(img);
    auto r = gen_bitstream_bytemode(1, "Hello");
    for (int i = 0; i<19; ++i){
        for (int j = 0; j < 8; ++j){
            std::cout << (int)((r[i] >> (7 - j)) & 1);
        }
    }
    std::cout << std::endl;
    auto gfr = add_gf_to_bitstream(r);
    for (auto el : gfr){
        std::cout << std::hex << "0x"<< (int)el << '\n';
    }
    std::cout << std::endl;
    draw_finder_7x7(0,0,field);
    draw_finder_7x7(0,14,field);
    draw_finder_7x7(14,0,field);
    draw_timing_patterns(field);
    field[(13 * FIELD_WIDTH) + 8] = BLACK; // Dark module
    draw_data(field, gfr);
    use_simple_mask(field);
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
