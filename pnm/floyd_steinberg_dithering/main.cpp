#include <iostream>
#include <fstream>
#include <bitset>
#include <climits>
#include <cstring>

#define HEIGHT 2000
#define WIDTH  2000
#define STR(x) #x
#define TO_STR(x) STR(x)
#define TO_HEADER(width,height) "P4\n" TO_STR(width) " " TO_STR(height) "\n"

float field[WIDTH * HEIGHT];
void __attribute__((constructor)) init_field(){
    for (int i = 0; i < (HEIGHT/3) * WIDTH; ++i)
        field[i] = 1.f;
    for (int i = (HEIGHT/3) * WIDTH; i < ((HEIGHT/3) * 2) * WIDTH; ++i)
        field[i] = 0.5f;
    for (int i = ((HEIGHT/3) * 2) * WIDTH; i < HEIGHT * WIDTH; ++i)
        field[i] = 1.f;
}

constexpr size_t SZ = HEIGHT * WIDTH;
constexpr const char* HEIGHT_STR = TO_STR(HEIGHT);
constexpr const char* WIDTH_STR = TO_STR(WIDTH);
constexpr const char* HEADER = TO_HEADER(WIDTH, HEIGHT);

unsigned char data[strlen(HEADER) + ((HEIGHT * WIDTH)/CHAR_BIT)];
void __attribute((constructor)) init_header() {
    strcpy((char*)data, HEADER);
}

void save_image(const std::string& name, const std::bitset<HEIGHT*WIDTH>& bs){
    size_t current_bit = ((strlen(HEADER)) * CHAR_BIT);
    for (int i = 0; i < SZ; ++i, ++current_bit)
        data[current_bit/CHAR_BIT] |= (bs[i] << (7-(current_bit%8)));
    auto file = std::ofstream(name, std::ios::binary);
    if (!file.is_open()) std::abort();
    file.write((char*)data, sizeof(data));
    file.close();
}

std::bitset<HEIGHT*WIDTH> gen_image_pixels(){
    std::bitset<HEIGHT*WIDTH> res{};
    float current_error = 0.f;
    for (int i = 0; i < SZ; ++i){
        res[i] = (field[i] > 0.5f);
        current_error = field[i] - (float)res[i];
        if ((i + 1) % WIDTH != 0 && i + 1 < SZ)
            field[i+1]       += (current_error * 7.f/16.f);
        if (i % WIDTH != 0 && i-1+WIDTH < SZ)
            field[i-1+WIDTH] += (current_error * 3.f/16.f);
        if (i+WIDTH < SZ)
            field[i+WIDTH]   += (current_error * 5.f/16.f);
        if ((i + 1) % WIDTH != 0 && i+WIDTH+1 < SZ)
            field[i+WIDTH+1] += (current_error * 1.f/16.f);
    }
    return res;
}

int main(){
    save_image("res.pbm", gen_image_pixels());
}