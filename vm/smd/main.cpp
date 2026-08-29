#include "raylib.h"
#include <iostream>
constexpr size_t HEIGHT = 800;
constexpr size_t WIDTH = 1000;


int main(){
    InitWindow(WIDTH, HEIGHT, "SegaMegaDrive");
    while(!WindowShouldClose()){
        BeginDrawing();
            ClearBackground(WHITE);
        EndDrawing();
    }
    CloseWindow();
}
