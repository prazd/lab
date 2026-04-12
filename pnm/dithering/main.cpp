#include <iostream>
#include <fstream>
#include <bitset>
#include <climits>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define IMAGE_PARSE 1

#define WIDTH  3800
#define HEIGHT 2527
#define STR(x) #x
#define TO_STR(x) STR(x)
#define TO_P4_HEADER(width,height) "P4\n" TO_STR(width) " " TO_STR(height) "\n"
#define TO_P6_HEADER(width,height) "P6\n" TO_STR(width) " " TO_STR(height) "\n255\n"

struct RGB{unsigned char r, g, b;};
constexpr RGB RED =  {200,0,0};
constexpr RGB BLUE = {0,0,200};

constexpr size_t SZ = HEIGHT * WIDTH;
constexpr const char* HEIGHT_STR = TO_STR(HEIGHT);
constexpr const char* WIDTH_STR = TO_STR(WIDTH);
constexpr const char* HEADER_P4 = TO_P4_HEADER(WIDTH, HEIGHT);
constexpr const char* HEADER_P6 = TO_P6_HEADER(WIDTH, HEIGHT);


unsigned char data_p4[strlen(HEADER_P4) + ((HEIGHT * WIDTH)/CHAR_BIT)];
void __attribute((constructor)) init_HEADER_P4() {
    strcpy((char*)data_p4, HEADER_P4);
}

unsigned char data_p6[strlen(HEADER_P6) + (HEIGHT * WIDTH) * 3];
void __attribute((constructor)) init_HEADER_P6() {
    strcpy((char*)data_p6, HEADER_P6);
}

void save_file(const std::string& name, char* data, size_t sz){
    auto file = std::ofstream(name, std::ios::binary);
    if (!file.is_open()) std::abort();
    file.write(data, sz);
    file.close();
}

float field[WIDTH * HEIGHT];
#if IMAGE_PARSE
    void __attribute__((constructor)) init_field(){
        int x,y,n;
        auto data = stbi_load("/Users/prazd/Desktop/books/cpp/cpp/pnm/dithering/img/david.jpg", &x, &y, &n, 0);
        size_t sz = (x * y);
        for (size_t i = 0, current = 0; i < sz; ++i, current+=3){
           field[i] = (0.299f * data[current] + 0.587f * data[current+1] + 0.114f * data[current+2])/255.f;
        }
        free(data);
    }
#else
void __attribute__((constructor)) init_field(){
    for (int i = 0; i < (HEIGHT/3) * WIDTH; ++i)
        field[i] = 1.f;
    for (int i = (HEIGHT/3) * WIDTH; i < ((HEIGHT/3) * 2) * WIDTH; ++i)
        field[i] = 0.5f;
    for (int i = ((HEIGHT/3) * 2) * WIDTH; i < HEIGHT * WIDTH; ++i)
        field[i] = 1.f;
}
#endif


void create_p6_image(const std::string& name, const std::bitset<HEIGHT*WIDTH>& bs, RGB c1, RGB c2){
    for (size_t i = 0, current_pos = ((strlen(HEADER_P6))); i < SZ; ++i, current_pos+=3){
        data_p6[current_pos]   = bs[i] ? c1.r : c2.r;
        data_p6[current_pos+1] = bs[i] ? c1.g : c2.g;
        data_p6[current_pos+2] = bs[i] ? c1.b : c2.b;
    }
    save_file(name, (char*)data_p6, sizeof(data_p6));
}

void create_p4_image(const std::string& name, const std::bitset<HEIGHT*WIDTH>& bs){
    size_t current_bit = ((strlen(HEADER_P4)) * CHAR_BIT);
    for (size_t i = 0; i < SZ; ++i, ++current_bit)
        data_p4[current_bit/CHAR_BIT] |= (bs[i] << (7-(current_bit%8)));
    save_file(name, (char*)(data_p4), sizeof(data_p4));
}


std::bitset<HEIGHT*WIDTH> floyd_steinberg_dithering(){
    std::bitset<HEIGHT*WIDTH> res{};
    float current_error = 0.f;
    for (size_t i = 0; i < SZ; ++i){
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

std::bitset<HEIGHT*WIDTH> atkinson_dithering(){
    std::bitset<HEIGHT*WIDTH> res{};
    float current_error = 0.f;
    for (size_t i = 0; i < SZ; ++i){
        // std::cout << "Fild[i]:" << field[i] << '\n';
        res[i] = (field[i] > 0.5f);
        current_error = field[i] - (float)res[i];
        if ((i + 1) % WIDTH != 0 && i + 1 < SZ)
            field[i+1]       += (current_error * 1.f/8.f);
        if (i % WIDTH < WIDTH-2 && i + 2 < SZ)
            field[i+2]       += (current_error * 1.f/8.f);
        if (i % WIDTH != 0 && i-1+WIDTH < SZ)
            field[i-1+WIDTH] += (current_error * 1.f/8.f);
        if (i+WIDTH < SZ)
            field[i+WIDTH]   += (current_error * 1.f/8.f);
        if (i+WIDTH*2 < SZ)
            field[i+WIDTH*2]   += (current_error * 1.f/8.f);
        if ((i + 1) % WIDTH != 0 && i+WIDTH+1 < SZ)
            field[i+WIDTH+1] += (current_error * 1.f/8.f);
    }
    return res;
}

int main(){
    auto pixels = floyd_steinberg_dithering();
    create_p4_image("p4.pbm", pixels);
    create_p6_image("p6.ppm", pixels, RED, BLUE);
}