#include <iostream>
#include <array>
#include <sstream>
#include "raylib.h"


constexpr size_t HEIGHT = 500;
constexpr size_t WIDTH = 1000;

template <size_t w, size_t h, size_t scale = 1>
struct Field {
    static constexpr size_t width = w/scale;
    static constexpr size_t height = h/scale;
    std::stringstream ss;
    bool run = false;
    bool step = false;
    enum Type {Empty = 0x1,Sand=0x2,Water=0x4,Stone=0x8};
    
    size_t sand_count = 0;
    size_t water_count = 0;
    size_t stone_count = 0;
    size_t empty_count = 0;

    Rectangle drawing_rec{0,0,3,3};
    Type d_type = Empty;
    Texture2D t;
    struct Cell {
        bool fresh = false;
        unsigned char state = 0x1;
        Type get() {
            return static_cast<Type>(state & (Empty | Sand | Stone | Water));
        }
        void set(Type t){
            state &= 0;
            state |= t;
        }
        void set_by_brush(Type t){
            set(t);
            fresh = true;
        }
        Color get_color() {
            switch(get()) {
                case Empty: return  GRAY;
                case Water: return  BLUE;
                case Sand:  return  YELLOW;
                case Stone: return  BLACK;
            }
        }
    };

    struct Intent {
        std::array<std::tuple<size_t, Type, Type>, 5> cells{};
        size_t current_sz = 0;
        void push(size_t el, Type t1, Type t2){

            cells[current_sz++] = std::tuple{el,t1,t2};
        }
        void clear(){
            current_sz = 0;
        }
        bool empty() {return current_sz == 0;}
        auto get() {
            if (current_sz == 1){
                return cells[0];
            } else {
                return cells[rand() % current_sz];
            }
        }
    };

    static inline std::bitset<height*width> in_intent{};
    static inline std::array<Color, height*width> buffer{};
    static inline Cell field[height * width]{};
    static inline Intent intents[height * width]{};

    Field() {
        auto img = GenImageColor(width, height, WHITE);
        t = LoadTextureFromImage(img);
        UnloadImage(img);

    }

    void add_to_intent(size_t i1, size_t i2, Type t1, Type t2) {
        // todo: update conflict resolving
        if (!in_intent[i1] && !in_intent[i2]) {
            intents[i1].push(i2, t1, t2);
            in_intent[i1] = true;
            in_intent[i2] = true;
        }
    }

    void next_d_type(){
        d_type = static_cast<Type>(((d_type << 1) | d_type >> 3) & 0b1111);
    }
    void draw(){
        for (size_t i = 0; i < height; ++i){
            for (size_t j = 0; j < width; ++j){
                buffer[(i * width) + j] = field[(i * width) + j].get_color();
            }
        }
        UpdateTexture(t, buffer.data());
    }
    
    void brush(){
        if (!(drawing_rec.x >= 0 && drawing_rec.x + drawing_rec.width <= width-1 && 
            drawing_rec.y >= 0 && drawing_rec.y + drawing_rec.height <= height-1)
        ) return;
        for (size_t i = drawing_rec.y, e = drawing_rec.y + drawing_rec.height; i != e; ++i){
            for (size_t j = drawing_rec.x, je = drawing_rec.x + drawing_rec.width; j != je; ++j){
                if (field[(i * width) + j].get() == Empty) field[(i * width) + j].set_by_brush(d_type);
            }
        }
    }

    void sand_update(size_t i, size_t j) {
        if (auto el = field[((i + 1) * width) + j].get(); el & (Empty | Water)){
            add_to_intent(((i + 1) * width) + j, (i * width) + j, Sand, el);
        } else if (( j != 0 && field[((i + 1) * width) + j - 1].get() & (Empty | Water))) {
            add_to_intent(((i + 1) * width) + j - 1, (i * width) + j, Sand, field[((i + 1) * width) + j - 1].get());
        } else if (j != width - 1 && field[((i + 1) * width) + j + 1].get() & (Empty | Water)) {
            add_to_intent(((i + 1) * width) + j + 1, (i * width) + j, Sand, field[((i + 1) * width) + j + 1].get());
        } 
    }

    void water_update(size_t i, size_t j) {
        if (field[((i + 1) * width) + j].get() & (Empty)){
            add_to_intent(((i + 1) * width) + j, (i * width) + j, Water, Empty);
        } else if ((j != 0 &&  field[((i + 1) * width) + j - 1].get() & (Empty))) {
            add_to_intent(((i + 1) * width) + j - 1, (i * width) + j, Water, Empty);
        } else if (j != width - 1 && field[((i + 1) * width) + j + 1].get() & (Empty)) {
            add_to_intent(((i + 1) * width) + j + 1, (i * width) + j, Water, Empty);
        } else if (j != width - 1 && field[((i) * width) + j + 1].get() & (Empty) && j != 0 && field[((i) * width) + j - 1].get() & (Empty)) {
            add_to_intent(((i) * width) + j + (rand() % 2 ? 1 : -1), (i * width) + j, Water, Empty);
        } else if (j != 0 && field[((i) * width) + j - 1].get() & (Empty)) {
            add_to_intent(((i) * width) + j - 1, (i * width) + j, Water, Empty);
        } else if (j != width - 1 && field[((i) * width) + j + 1].get() & (Empty)) {
            add_to_intent(((i) * width) + j + 1, (i * width) + j, Water, Empty);
        }
    }

    void update_counters(){
        for (size_t i = 0, e = width * height; i != e; ++i){
            switch (field[i].get()) {
                case Empty: ++empty_count; break;
                case Stone: ++stone_count; break;
                case Water: ++water_count; break;
                case Sand:   ++sand_count; break;
            }
        }
    }
    void clear_counters(){
        empty_count = stone_count = water_count = sand_count = 0;
    }

    void draw_text() {
        ss << "WaterCount:" << water_count << '\n';
        ss << "SandCount:"  << sand_count << '\n';
        ss << "EmptyCount:" << empty_count << '\n';
        ss << "StoneCount:" << stone_count << '\n';
        auto s = ss.str();
        DrawText(s.c_str(), 0, 0, 12.f, BLACK);
        ss.str("");
    }

    void commit(){
#if 1
        for (int i = height-1, e = -1; i != e; --i){
            for (size_t j = 0, je = width; j != je; ++j){
                if (field[(i * width) + j].fresh) field[(i * width) + j].fresh = false;
                if (!intents[(i * width) + j].empty()){
                    auto [index,t1,t2] = intents[(i * width) + j].get();
                    field[index].set(t2);
                    field[(i * width) + j].set(t1);
                    intents[(i * width) + j].clear();
                }
            }
        }
#else
        for (size_t i = 0, e = height; i != e; ++i){
            for (size_t j = 0, je = width; j != je; ++j){
                if (field[(i * width) + j].fresh) field[(i * width) + j].fresh = false;
                if (!intents[(i * width) + j].empty()){
                    auto [index,t1,t2] = intents[(i * width) + j].get();
                    field[index].set(t2);
                    field[(i * width) + j].set(t1);
                    intents[(i * width) + j].clear();
                }
            }
        }
#endif
    }

    void update() {
        for (size_t i = 0, e = height; i != e; ++i){
            for (size_t j = 0, je = width; j != je; ++j){
                auto index = (i * width) + j;
                if (field[index].get() == Sand && !field[index].fresh && i != height-1)
                    sand_update(i, j);
                if (field[index].get() == Water && !field[index].fresh && i != height-1)
                    water_update(i, j);
            }
        }
        commit();
    }

    void reset(){
        std::fill(std::begin(field), std::end(field), Cell{});
    }
    void handle() {
        in_intent = 0;
        clear_counters();
        if (IsKeyDown(KEY_U)) {drawing_rec.width += 1; drawing_rec.height += 1;}
        if (IsKeyDown(KEY_S) || IsKeyPressed(KEY_A)) step = true;
        if (IsKeyDown(KEY_Y)) {drawing_rec.width -= drawing_rec.width > 3 ? 1 : 0; drawing_rec.height -=  drawing_rec.height > 3 ? 1 : 0;}
        if (IsKeyPressed(KEY_M)) next_d_type();
        if (IsKeyPressed(KEY_C)) {reset();}
        if (IsKeyPressed(KEY_R)) run = !run;
        drawing_rec.x = GetMouseX()/scale-drawing_rec.width/2;
        drawing_rec.y = GetMouseY()/scale-drawing_rec.height/2;
        if (run || step) update();
        draw();
        update_counters();
        if (IsMouseButtonDown(0)) {brush();}
        if (step) step = false;
    }
};


int main(){
    InitWindow(WIDTH, HEIGHT, "Elemental");
    Field<WIDTH,HEIGHT,2> field;
    while(!WindowShouldClose()){
        field.handle();
        BeginDrawing();
            ClearBackground(WHITE);
            DrawTexturePro(field.t, {0, 0, (float)field.t.width, (float)field.t.height},  {0, 0,WIDTH, HEIGHT}, {0, 0},0, WHITE);
            DrawFPS(0,0);
            field.draw_text();
        EndDrawing();
    }
    CloseWindow();
}