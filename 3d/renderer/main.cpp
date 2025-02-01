#include <iostream>
#include "tgaimage.hpp"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);

void draw_line(int x0, int y0, int x1, int y1, TGAImage& img, const TGAColor& color) {
    for (float t = 0.0f; t < 1.0f; t+=0.01f) {
        int x = x0 + (x1-x0)*t;
        int y = y0 + (y1-y0)*t;
        img.set(x,y,color);
    }
}

int main() {
    TGAImage image(100,100, TGAImage::RGB);
    draw_line(10,10,30,30, image, red);
    draw_line(50,10,30,30, image, red);
    draw_line(10,10,50,10, image, red);
    image.flip_vertically();
    image.write_tga_file("output.tga");
}