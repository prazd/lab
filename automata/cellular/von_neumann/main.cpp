#include <iostream>
#include "raylib.h"

constexpr size_t WIDTH = 800;
constexpr size_t HEIGHT = 800;
constexpr size_t FIELD_WIDTH = 40;
constexpr size_t FIELD_HEIGHT = 40;

struct Automata {
    struct Cell {
       enum State: unsigned char {
            U, // base
            S, S0, S00, S01, S000, S1, S10, S11,
            C00, C10, C01, C11, 
            T00, T01, T02, T03, // right/up/left/down
            T10, T11, T12, T13  // special right/up/left/down
       };
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

int main(){
    InitWindow( WIDTH, HEIGHT, "VNA");
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
        EndDrawing();
    }
    CloseWindow();
}