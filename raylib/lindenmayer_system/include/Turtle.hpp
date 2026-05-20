#ifndef TURTLE_H
#define TURTLE_H
#include <array>
#include <stack>
#include "raylib.h"

template <size_t width, size_t height, size_t x, size_t y>
struct Turtle2D {
    float step = 1.f;
    float s_angle = 120;
    Texture2D target;
    static inline std::array<Color, width * height> buffer; 

    enum Commands: char {ForwardDraw = 'F', Forward = 'f', Plus = '+', Minus = '-', Push = '[', Pop = ']', GDraw = 'G'};
    struct State {
        Vector2 vec{};
        float angle{0};
    } state;
    std::stack<State> s;

    Turtle2D(): state({Vector2{x,y}}) {
        Image img = GenImageColor(width, height, WHITE);
        target = LoadTextureFromImage(img);
        UnloadImage(img);
        clear_texture();
    }

    void update_texture(){
        UpdateTexture(target, buffer.data());
    }

    void clear_texture(){
        std::fill(std::begin(buffer), std::end(buffer), WHITE);
        update_texture();
    }

    void draw_line(){
        auto next = Vector2{state.vec.x + step * std::cos((PI * state.angle)/180), state.vec.y + step * std::sin((PI * state.angle)/180)};
        size_t current_x = state.vec.x;
        size_t current_y = state.vec.y;
        if (current_x < width && current_y < height)
            buffer[(current_y * width) + current_x] = BLACK;
        state.vec = next;
    }
    void handle(char sym){
        switch(sym) {
            case GDraw:
            case ForwardDraw: {draw_line(); break;}
            case Forward:     {state.vec = Vector2{state.vec.x - step * std::cos((PI * state.angle)/180), state.vec.y - step * std::sin((state.angle * PI)/180)}; break;}
            case Plus:        {state.angle+=s_angle; break;}
            case Minus:       {state.angle-=s_angle; break;}
            case Push:        {s.push(state); break;}
            case Pop:         {state = s.top(); s.pop(); break;}
        }
    }
};
#endif