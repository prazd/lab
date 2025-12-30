#include <iostream>
#include "raylib.h"

constexpr size_t WIDTH = 400;
constexpr size_t HEIGHT = 400;

int main(){
    InitWindow(WIDTH, HEIGHT, "NES");
    while(!WindowShouldClose()){
        BeginDrawing();
            ClearBackground(WHITE);
        EndDrawing();
    }
    CloseWindow();
}
