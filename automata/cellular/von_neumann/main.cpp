#include <iostream>
#include <array>
#include <algorithm>
#include "raylib.h"

constexpr size_t WIDTH = 800;
constexpr size_t HEIGHT = 800;
constexpr size_t FIELD_WIDTH = 40;
constexpr size_t FIELD_HEIGHT = 40;
constexpr size_t WIDTH_N = WIDTH/FIELD_WIDTH;
constexpr size_t HEIGHT_N = HEIGHT/FIELD_HEIGHT;

enum State: unsigned char {
        Empty,
        U, // base
        S, S0, S00, S01, S000, S1, S10, S11,
        C00, C10, C01, C11, 
        T00, T01, T02, T03, // right/up/left/down
        T10, T11, T12, T13  // special right/up/left/down
};

enum Direction: unsigned char {UP,DOWN,LEFT,RIGHT};

static constexpr State transport[] = {
    T00, T01, T02, T03, // right/up/left/down
    T10, T11, T12, T13  // special right/up/left/down
};

auto figure_by_state(Direction d, float x_w, float y_w){
    float x3 = x_w/2;
    float y3 = y_w/4;
    float x2 = 0;
    float y2 = -y_w/4;
    float x1 = -x_w/2;
    float y1 = y_w/4;
    switch (d){
        // TODO: check
        case UP:    return std::tuple(Vector2{x3,y3}, Vector2{x2,y2}, Vector2{x1,y1});       //         {x,   y}
        case DOWN:  return std::tuple(Vector2{-x3,-y3}, Vector2{-x2,-y2}, Vector2{-x1,-y1}); // 180:    {-x, -y}
        case LEFT:  return std::tuple(Vector2{-y3, x3}, Vector2{-y2,x2}, Vector2{-y1, x1});  // 90 <-:  {-y,  x}
        case RIGHT: return std::tuple(Vector2{y3,-x3}, Vector2{y2,-x2}, Vector2{y1,-x1});    // 90 ->:  {y,  -x}
    }
}

struct Automata {
    struct Cell {
       State next(State current, State up, State down, State left, State right){
            bool is_signal = (down == T01 || down == T11 || up == T03 || up == T13 || 
                        left == T00 || left == T10 || right == T02 || right == T12);
            switch (current) {
                case U:    return is_signal ? S : U;
                case S:    return is_signal ? S1  : S0;
                case S0:   return is_signal ? S00 : S01;
                case S00:  return is_signal ? T02 : S000;
                case S000: return is_signal ? T01 : T00;
                case S01:  return is_signal ? T10 : T03;
                case S1:   return is_signal ? S11 : S10;
                case S10:  return is_signal ? T12 : T11;
                case S11:  return is_signal ? C00 : T13;
            }
            // TODO: finish
       }
    };
    State field[WIDTH_N * HEIGHT_N]{};
};

void draw_figure_by_state(State s, Rectangle rec, float offset){
    if (std::any_of(std::begin(transport), std::end(transport), [s](auto el){return el == s;})) {
    Direction d;
    switch(s){
        case T13:
        case T03: d = DOWN; break;
        case T12:
        case T02: d = LEFT; break;
        case T10:
        case T00: d = RIGHT; break;
        case T11:
        case T01: d = UP; break;
    }
    auto [a3,a2,a1] = figure_by_state(d, rec.width, rec.height);
    DrawTriangle({a3.x + rec.x + offset, a3.y + rec.y + offset}, {a2.x + rec.x + offset, a2.y + rec.y + offset}, {a1.x + rec.x + offset, a1.y + rec.y + offset}, BLUE);
    }
}

struct SelectMenu {
    static constexpr float offset = 16;
    State state = U;
    bool active = false;
    Rectangle menu = {0,0,200,200};
    State arr[5][8]{
        {U},
        {S, S0, S00, S01, S000, S1, S10, S11},
        {C00, C10, C01, C11},
        {T00, T01, T02, T03},
        {T10, T11, T12, T13},
    };
    const char* arr_str[5][8]{
        {"U"},
        {"S", "S0", "S00", "S01", "S000", "S1", "S10", "S11"},
        {"C00", "C10", "C01", "C11"},
        {"T00", "T01", "T02", "T03"},
        {"T10", "T11", "T12", "T13"},
    };
    
    void draw(){
        if (IsKeyPressed(KEY_M)) {
            active = !active;
            if (active) {
                auto mouse = GetMousePosition();
                menu = Rectangle{mouse.x, mouse.y, menu.width, menu.height};
            }
        }
        if (!active) return;
        DrawRectangle(menu.x, menu.y, menu.width, menu.height, WHITE);
        float x = menu.x;
        float y = menu.y;
        float x_w = 200/8;
        float y_w = 200/5;
        Rectangle rec;
        for (size_t i = 0; i < 5; ++i){
            for (size_t j = 0; j < 8; ++j){
                rec = {x,y,x_w,y_w};
                if (CheckCollisionPointRec(GetMousePosition(), rec)){
                    if (IsMouseButtonPressed(0)) state = arr[i][j];
                    else DrawRectangleLinesEx(rec, 1.f, BLUE);
                } 
                if (state == arr[i][j]) DrawRectangleLinesEx(rec, 1.f, GREEN);
                draw_figure_by_state(arr[i][j],{x, y, x_w, y_w}, offset);
                DrawText(arr_str[i][j], x, y, 10.f, BLACK);
                x += x_w;
            }
            x = menu.x;
            y += y_w;
        }
        
    }
};

int main(){
    InitWindow(WIDTH, HEIGHT, "VNA");
    SelectMenu sm;
    Automata a;
    auto grid = []{
        RenderTexture2D result = LoadRenderTexture(WIDTH, HEIGHT);
        BeginTextureMode(result);
            ClearBackground(WHITE);
            for (size_t i = 0; i < HEIGHT; i+=FIELD_HEIGHT)
                DrawLine(0, i, WIDTH, i, BLACK);
            for (size_t i = 0; i < WIDTH; i+=FIELD_WIDTH)
                DrawLine(i, 0, i, HEIGHT, BLACK);
        EndTextureMode();
        return result;
    }();
    while(!WindowShouldClose()){
        BeginDrawing();
            ClearBackground(WHITE);
            DrawTextureRec(grid.texture, {0, 0, (float)grid.texture.width, -(float)grid.texture.height}, {0, 0}, WHITE);
            for (size_t i = 0; i < HEIGHT_N; ++i){
                for (size_t j = 0; j < WIDTH_N;++j){
                    auto pos = (i * WIDTH_N) + j;
                    auto rec = Rectangle{float(i * FIELD_HEIGHT), float(j * FIELD_WIDTH), FIELD_WIDTH, FIELD_HEIGHT};
                    if (!sm.active && CheckCollisionPointRec(GetMousePosition(), rec) && IsMouseButtonPressed(0))
                        a.field[pos] = sm.state;                    
                    draw_figure_by_state(a.field[pos], rec, 20);
                   
                }
            }
            sm.draw();
        EndDrawing();
    }
    CloseWindow();
}