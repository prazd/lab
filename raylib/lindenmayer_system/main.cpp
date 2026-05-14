#include <iostream>
#include <bitset>
#include <stack>
#include "raylib.h"

constexpr float HEIGHT = 1000;
constexpr float WIDTH = 1500;

struct Turtle {
    static constexpr float step = 1.f;
    static constexpr float s_angle = 60;
    RenderTexture2D texture;

    enum Commands: char {ForwardDraw = 'F', Forward = 'f', Plus = '+', Minus = '-', Push = '[', Pop = ']'};
    struct State {
        Vector2 vec{};
        float angle{0};
    } state;
    std::stack<State> s;

    Turtle(float width, float height, float x, float y): texture(LoadRenderTexture(width, height)), state({Vector2{x,y}}) {}
    void handle(char sym){
        switch(sym) {
            case ForwardDraw: {
                BeginTextureMode(texture);
                    auto next = Vector2{state.vec.x + step * std::cos((PI * state.angle)/180), state.vec.y + step * std::sin((PI * state.angle)/180)};
                    DrawLineEx(state.vec, next, 1.f, BLACK);
                    state.vec = next;
                EndTextureMode();
                break;}
            case Forward: {state.vec = Vector2{state.vec.x - step * std::cos((PI * state.angle)/180), state.vec.y - step * std::sin((state.angle * PI)/180)}; break;}
            case Plus:    {state.angle+=s_angle; break;}
            case Minus:   {state.angle-=s_angle; break;}
            case Push:    {s.push(state); break;}
            case Pop:     {state = s.top(); s.pop(); break;}
        }
    }
    ~Turtle(){UnloadRenderTexture(texture);}
};

int main(){
    InitWindow(WIDTH, HEIGHT, "LindenmayerSystem");
    Turtle t(WIDTH, HEIGHT, WIDTH/2, HEIGHT/2);
    std::string s = "F";
#if 1
    std::string rule = "F+F--F+F";
#else    
    std::string rule = "F[+F]F[-F]F";
#endif    
    std::string tmp;
    for (int i = 0; i < 6; ++i){
        tmp = "";
        for (auto el: s) {
            if (el == 'F') {
                tmp += rule;
            } else {
                tmp += el;
            }
        }
        s = std::move(tmp);
    }
    for (auto el: s) t.handle(el);

    while(!WindowShouldClose()){
        BeginDrawing();
                ClearBackground(WHITE);
                DrawTextureRec(t.texture.texture, {0, 0, (float)t.texture.texture.width, -(float)t.texture.texture.height}, {0, 0}, WHITE);
        EndDrawing();
    }
    CloseWindow();
}