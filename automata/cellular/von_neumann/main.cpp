#include <iostream>
#include <array>
#include "raylib.h"

constexpr size_t WIDTH = 800;
constexpr size_t HEIGHT = 800;
constexpr size_t FIELD_WIDTH = 40;
constexpr size_t FIELD_HEIGHT = 40;

enum State: unsigned char {
        Empty,
        U, // base
        S, S0, S00, S01, S000, S1, S10, S11,
        C00, C10, C01, C11, 
        T00, T01, T02, T03, // right/up/left/down
        T10, T11, T12, T13  // special right/up/left/down
};

enum Direction: unsigned char {UP,DOWN,LEFT,RIGHT};

auto triangle_by_pos(Direction d){
    switch (d){
        // TODO: check
        case UP:    return std::tuple(Vector2{10,10}, Vector2{0,0}, Vector2{-10,10});
        case DOWN:  return std::tuple(Vector2{10,0}, Vector2{-10,0}, Vector2{0,10});
        case LEFT:  return std::tuple(Vector2{0,10}, Vector2{10,20}, Vector2{10,0});
        case RIGHT: return std::tuple(Vector2{0,0},  Vector2{0,20}, Vector2{10,10});
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
    Cell field[FIELD_HEIGHT * FIELD_WIDTH];
};

struct SelectMenu {
    static constexpr float offset = 10;
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
                
                if (arr[i][j] == T11 || arr[i][j] == T01) {
                    auto [a3,a2,a1] = triangle_by_pos(UP);
                    DrawTriangle({a3.x + x + offset, a3.y + y + offset}, {a2.x + x + offset, a2.y + y + offset}, {a1.x + x + offset, a1.y + y + offset}, BLUE);
                } else if (arr[i][j] == T13 || arr[i][j] == T03){
                    auto [a3,a2,a1] = triangle_by_pos(DOWN);
                    DrawTriangle({a3.x + x + offset, a3.y + y + offset}, {a2.x + x + offset, a2.y + y + offset}, {a1.x + x + offset, a1.y + y + offset}, BLUE);
                } else if (arr[i][j] == T12 || arr[i][j] == T02){
                     auto [a3,a2,a1] = triangle_by_pos(LEFT);
                    DrawTriangle({a3.x + x + offset, a3.y + y + offset}, {a2.x + x + offset, a2.y + y + offset}, {a1.x + x + offset, a1.y + y + offset}, BLUE);
                } else if (arr[i][j] == T10 || arr[i][j] == T00){
                    auto [a3,a2,a1] = triangle_by_pos(RIGHT);
                    DrawTriangle({a3.x + x + offset, a3.y + y + offset}, {a2.x + x + offset, a2.y + y + offset}, {a1.x + x + offset, a1.y + y + offset}, BLUE);
                }
                DrawText(arr_str[i][j], x, y, 10.f, BLACK);
                x += x_w;
            }
            x = menu.x;
            y += y_w;
        }
        
    }
};

int main(){
    InitWindow( WIDTH, HEIGHT, "VNA");
    SelectMenu sm;
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
            sm.draw();
        EndDrawing();
    }
    CloseWindow();
}