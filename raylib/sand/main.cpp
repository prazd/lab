#include <iostream>
#include "raylib.h"
#include <array>

constexpr size_t HEIGHT = 500.f;
constexpr size_t WIDTH = 1000.f;

template <size_t width, size_t height>
struct Field {
    bool run = false;
    enum Type {Empty = 0x0,Sand=0x1,Stone=0x2};
    static inline constexpr Type next_type[3] = {Sand, Stone, Empty};
    Rectangle drawing_rec{0,0,10,10};
    Type d_type = Empty;
    bool should_update = true;
    Texture2D t;
    struct Cell {
        unsigned char state;
        Type get() {
            return static_cast<Type>(state & (Empty | Sand | Stone));
        }
        void set(Type t){
            state &= 0;
            state |= t;
        }
        Color get_color() {
            switch(get()) {
                case Empty: return  BLUE;
                case Sand:  return  YELLOW;
                case Stone: return  BLACK;
            }
        }
    };

    struct Intent {
        std::array<size_t, 3> cells{};
        size_t current_sz = 0;
        void push(size_t el){
            cells[current_sz++] = el;
        }
        void clear(){
            current_sz = 0;
        }
        bool empty() {return current_sz == 0;}
        auto get_index() {
            if (current_sz == 1){
                return cells[0];
            } else {
                return cells[rand() % current_sz];
            }
        }
    };
    static inline std::array<Color, height*width> buffer{};
    static inline Cell field[height * width]{};
    static inline Intent intents[height * width]{};

    Field() {
        auto img = GenImageColor(width, height, WHITE);
        t = LoadTextureFromImage(img);
        UnloadImage(img);

    }
    void draw(){
        for (size_t i = 0; i < height; ++i){
            for (size_t j = 0; j < width; ++j){
                buffer[(i * width) + j] = field[(i * width) + j].get_color();
            }
        }
        UpdateTexture(t, buffer.data());
    }
    void update_cells(){
        for (size_t i = drawing_rec.y, e = drawing_rec.y + drawing_rec.height; i != e; ++i){
            for (size_t j = drawing_rec.x, je = drawing_rec.x + drawing_rec.width; j != je; ++j){
                field[(i * width) + j].set(d_type);
            }
        }
    }
    void update_sand() {
        for (size_t i = 0, e = height; i != e; ++i){
            for (size_t j = 0, je = width; j != je; ++j){
                if (field[(i * width) + j].get() == Sand) {
                    if (i != height - 1){
                        if (field[((i + 1) * width) + j].get() == Empty){
                            intents[((i + 1) * width) + j].push((i * width) + j);
                        } else if ((j != 0 &&  field[((i + 1) * width) + j - 1].get() == Empty)) {
                            intents[((i + 1) * width) + j - 1].push((i * width) + j);
                        } else if (j != width - 1 && field[((i + 1) * width) + j + 1].get() == Empty) {
                            intents[((i + 1) * width) + j + 1].push((i * width) + j);
                        } 
                    }
                }
            }
        }
        
        for (size_t i = 0, e = height; i != e; ++i){
            for (size_t j = 0, je = width; j != je; ++j){
                if (!intents[(i * width) + j].empty()){
                    auto index = intents[(i * width) + j].get_index();
                    field[index].set(Empty);
                    field[(i * width) + j].set(Sand);
                    intents[(i * width) + j].clear();
                }
            }
        }
    }
    void reset(){
        std::fill(std::begin(field), std::end(field), Cell{});
    }
    void handle() {
        if (IsKeyPressed(KEY_M)) d_type = next_type[d_type];
        if (IsKeyPressed(KEY_C)) {reset(); should_update = true;}
        if (IsKeyPressed(KEY_R)) run = !run;
        drawing_rec.x = GetMouseX()-drawing_rec.width/2;
        drawing_rec.y = GetMouseY()-drawing_rec.height/2;
        if (IsMouseButtonDown(0)) {update_cells(); should_update = true;}
        if (run) update_sand();
        draw();
    }
};


int main(){
    InitWindow(WIDTH, HEIGHT, "SandSim");
    Field<WIDTH,HEIGHT> field;
    while(!WindowShouldClose()){
        field.handle();
        BeginDrawing();
            ClearBackground(WHITE);
            DrawTexturePro(field.t, {0, 0, (float)field.t.width, (float)field.t.height},  {0, 0,WIDTH, HEIGHT}, {0, 0},0, WHITE);
            DrawFPS(0,0);
        EndDrawing();
    }
    CloseWindow();
}