#include <iostream>
#include <vector>
#include "raylib.h"
#include <immintrin.h>

constexpr size_t DISPLAY_HEIGHT = 2048;
constexpr size_t DISPLAY_WIDTH = 2048;
size_t HEIGHT = DISPLAY_HEIGHT;
size_t WIDTH = DISPLAY_WIDTH;

struct Field {
    Texture2D display_texture;
    std::vector<Color> buffer;
    std::vector<unsigned char> cells_ncount;
    std::vector<unsigned char> cells_alive;

    void set_state(size_t i, size_t j, bool state){
        cells_alive[(i * WIDTH) + j] = state;
    }

    void next_gen(size_t i, size_t j){
        size_t index = (i * WIDTH) + j;
        bool alive = cells_alive[index];
        unsigned char alive_neighbours = cells_ncount[index];
        if (!alive && alive_neighbours == 3) {
            cells_alive[index] = 1;
        } else if (alive && (alive_neighbours < 2 || alive_neighbours > 3)) {
            cells_alive[index] = 0;
        }
    }

    #define LEFT_NEIGHBOUR(i,j)       cells_alive[(i * WIDTH) + (j - 1 + WIDTH) % WIDTH]
    #define RIGHT_NEIGHBOUR(i,j)      cells_alive[(i *  WIDTH) + (j + 1) % WIDTH]
    #define UP_NEIGHBOUR(i,j)         cells_alive[((i - 1 + HEIGHT) % HEIGHT) * WIDTH + j]
    #define DOWN_NEIGHBOUR(i,j)       cells_alive[((i + 1) % HEIGHT) * WIDTH + j]

    #define LEFT_UP_NEIGHBOUR(i,j)    cells_alive[((i - 1 + HEIGHT) % HEIGHT) * WIDTH + (j - 1 + WIDTH) % WIDTH]
    #define RIGHT_UP_NEIGHBOUR(i,j)   cells_alive[((i - 1 + HEIGHT) % HEIGHT) * WIDTH + (j + 1) % WIDTH]
    #define DOWN_LEFT_NEIGHBOUR(i,j)  cells_alive[((i + 1) % HEIGHT) * WIDTH + (j - 1 + WIDTH) % WIDTH]
    #define DOWN_RIGHT_NEIGHBOUR(i,j) cells_alive[((i + 1) % HEIGHT) * WIDTH + (j + 1) % WIDTH]


    // L1 [a0 a1 a2 a3 ...]
    // L2 [b0 b1 b2 b3 ...]
    // L3 [c0 c1 c2 c3 ...]

    // v1 - [a0 a1 a2 a3 ...] - L1[0]
    // v2 - [a1 a2 a3 a4 ...] - L1[1]
    // v3 - [a2 a3 a4 a5 ...] - L1[2]

    // v4 - [b0 b1 b2 b3 ...] - L2[0]
    // v5 - [b2 b3 b4 b5 ...] - L2[2]
    // v6 - [c0 c1 c2 c3 ...] - L3[0]
    // v7 - [c1 c2 c3 c4 ...] - L3[1]
    // v8 - [c2 c3 c4 c5 ...] - L3[2]
    void count_neighbors_batch(size_t i, size_t j){
        __m256i v1;
        __m256i v2;
        __m256i v3;
        __m256i v4;
        __m256i v5;
        __m256i v6;
        __m256i v7;
        __m256i v8;
        __m256i res;

        v1 = _mm256_loadu_si256((const __m256i*)(cells_alive.data() + (i - 1) * WIDTH + j));
        v2 = _mm256_loadu_si256((const __m256i*)(cells_alive.data() + (i - 1) * WIDTH + j + 1));
        v3 = _mm256_loadu_si256((const __m256i*)(cells_alive.data() + (i - 1) * WIDTH + j + 2));
        v4 = _mm256_loadu_si256((const __m256i*)(cells_alive.data() + (i * WIDTH + j)));
        v5 = _mm256_loadu_si256((const __m256i*)(cells_alive.data() + (i * WIDTH + j + 2)));
        v6 = _mm256_loadu_si256((const __m256i*)(cells_alive.data() + (i + 1) * WIDTH + j));
        v7 = _mm256_loadu_si256((const __m256i*)(cells_alive.data() + (i + 1) * WIDTH + j + 1));
        v8 = _mm256_loadu_si256((const __m256i*)(cells_alive.data() + (i + 1) * WIDTH + j + 2));

        res = _mm256_add_epi8(v1, v2);
        res = _mm256_add_epi8(res, v3);
        res = _mm256_add_epi8(res, v4);
        res = _mm256_add_epi8(res, v5);
        res = _mm256_add_epi8(res, v6);
        res = _mm256_add_epi8(res, v7);
        res = _mm256_add_epi8(res, v8);

        _mm256_storeu_si256((__m256i*)(cells_ncount.data() + i * WIDTH + j + 1), res);
    }

    void count_neighbors(size_t i, size_t j){
        unsigned char res = 0;
        res += LEFT_NEIGHBOUR(i,j);
        res += RIGHT_NEIGHBOUR(i,j);
        res += UP_NEIGHBOUR(i,j);
        res += DOWN_NEIGHBOUR(i,j);

        res += LEFT_UP_NEIGHBOUR(i,j);
        res += RIGHT_UP_NEIGHBOUR(i,j);
        res += DOWN_LEFT_NEIGHBOUR(i,j);
        res += DOWN_RIGHT_NEIGHBOUR(i,j);
        cells_ncount[(i * WIDTH) + j] = res;
    }

    void resize(){
        cells_ncount.resize(HEIGHT * WIDTH);
        cells_alive.resize(HEIGHT * WIDTH);
        buffer.resize(HEIGHT * WIDTH);
        auto img = GenImageColor(WIDTH, HEIGHT, WHITE);
        display_texture = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    Field(){resize();}

    void next() {
#if 1
        // TODO: check
        for (size_t i = 1; i < HEIGHT-1; ++i)
            for (size_t j = 0; j < WIDTH-1; j+=31)
                count_neighbors_batch(i, j);
#else
       for (size_t i = 0; i < HEIGHT; ++i)
            for (size_t j = 0; j < WIDTH; ++j)
                count_neighbors(i,j);
#endif
                

        for (size_t i = 0; i < HEIGHT; ++i)
            for (size_t j = 0; j < WIDTH; ++j)
                next_gen(i, j);
    }
};

int main(){
    bool pause = true;

    InitWindow(DISPLAY_WIDTH, DISPLAY_HEIGHT, "GoL");
    Field f;


    std::vector<Color> buffer;

    float CELL_SZ_X = DISPLAY_WIDTH/WIDTH;
    float CELL_SZ_Y = DISPLAY_HEIGHT/HEIGHT;

     auto update_texture_state = [&]{
        for (size_t i = 0; i < HEIGHT; ++i){
            for (size_t j = 0; j < WIDTH; ++j)
                f.buffer[i * WIDTH + j] = f.cells_alive[(i * WIDTH) + j] ? BLACK : WHITE;
        }
        UpdateTexture(f.display_texture, f.buffer.data());
     };

     bool mode = true;

    while (!WindowShouldClose())
    {
        if (!pause) f.next();

        update_texture_state();
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexturePro(f.display_texture, {0, 0, (float)f.display_texture.width, (float)f.display_texture.height},  {0, 0,(float)GetScreenWidth(), (float)GetScreenHeight()}, {0, 0},0, WHITE);
            DrawFPS(8, 8);
        EndDrawing();

        if (IsKeyPressed(KEY_P)){
            pause = !pause;
        }
        if (IsKeyPressed(KEY_M)) {
            mode = !mode;
        }

        if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_W)){
            HEIGHT = std::min(HEIGHT + (IsKeyPressed(KEY_Q) ? 8 : -8), DISPLAY_HEIGHT/2);
            WIDTH = std::min(WIDTH + (IsKeyPressed(KEY_Q) ? 8 : -8), DISPLAY_WIDTH/2);
            CELL_SZ_X = DISPLAY_WIDTH/WIDTH;
            CELL_SZ_Y = DISPLAY_HEIGHT/HEIGHT;
            f.resize();
        }
            
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            size_t x = GetMouseX()/CELL_SZ_X;
            size_t y = GetMouseY()/CELL_SZ_Y;
            if (f.cells_alive[(y * WIDTH) + x] != mode)
                f.set_state(y, x, mode);
        }
    }
    
    // UnloadRenderTexture(f.display_texture);
    CloseWindow();
    return 0;
}