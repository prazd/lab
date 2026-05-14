#include <iostream>
#include <bitset>

constexpr size_t HEIGHT = 25;
constexpr size_t WIDTH = 40;

template <size_t height, size_t width>
consteval auto midpoint_circle(int x_pos, int y_pos, int r) {
    std::bitset<height * width> field;
    int x = 0;
    int y = r;
    int d = 1 - r;
    while(x <= y) {
        field[((y_pos + y) * width) + x_pos + x] = 1;         // (x,  y)
        field[((y_pos + x) * width) + x_pos + y] = 1;         // (y,  x)
        field[((y_pos + y) * width) + (x_pos - x)] = 1;       // (-x, y)
        field[((y_pos + x) * width) + (x_pos - y)] = 1;       // (-y, x)

        field[((y_pos - y) * width) + (x_pos - x)] = 1;       // (-x, -y) 
        field[((y_pos - x) * width) + (x_pos - y)] = 1;       // (-y, -x)
        field[((y_pos-y) * width) + x_pos + x] = 1;           // (x,  -y)
        field[((y_pos-x) * width) + x_pos + y] = 1;           // (y,  -x)

        
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            --y;
        } 
        x+=1;
    }
    return field;
}

template <size_t height, size_t width>
void print_field(const auto& field) {
    for (size_t i = 0; i < height; ++i){
        for (size_t j = 0; j < width; ++j)
            std::cout << (field[(i * width) + j] ? '0' : '.');
        std::cout << '\n';
    }
}

int main(){
    print_field<HEIGHT, WIDTH>(midpoint_circle<HEIGHT, WIDTH>(20, 12, 8));
}