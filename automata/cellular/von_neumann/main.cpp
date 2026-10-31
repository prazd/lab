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
constexpr size_t STATE_W = 8;
constexpr size_t STATE_H = 7;
constexpr float SELECT_MENU_W = 200;
constexpr float SELECT_MENU_H = 200;

enum State: unsigned char {
        Empty,
        U, // base
        S, S0, S00, S01, S000, S1, S10, S11,
        C00, C10, C01, C11, 
        T00, T01, T02, T03,       // right/up/left/down
        T00A, T01A, T02A, T03A,   // active right/up/left/down
        T10, T11, T12, T13,        // special right/up/left/down
        T10A, T11A, T12A, T13A    // special  active right/up/left/down
};


enum Direction: unsigned char {UP,DOWN,LEFT,RIGHT};

static constexpr State transport[] = {
    T00, T01, T02, T03,   // right/up/left/down
    T00A, T01A, T02A, T03A,   // right/up/left/down
    T10, T11, T12, T13,    // special right/up/left/down
    T10A, T11A, T12A, T13A    // special right/up/left/down
};

static constexpr State transport_normal[] = {
    T00, T01, T02, T03,         // right/up/left/down
};

static constexpr State transport_normal_active[] = {
    T00A, T01A, T02A, T03A,   // right/up/left/down
};

static constexpr State transport_special[] = {
    T10, T11, T12, T13,         // special right/up/left/down
};

static constexpr State transport_special_active[] = {
    T10A, T11A, T12A, T13A,    // special right/up/left/down
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
        case LEFT:   return std::tuple(Vector2{y3,-x3}, Vector2{y2,-x2}, Vector2{y1,-x1});    // 90:  {y,  -x}
        case RIGHT: return std::tuple(Vector2{-y3, x3}, Vector2{-y2,x2}, Vector2{-y1, x1});   // 90:  {-y,  x}
    }
}

struct Automata {
    struct Cell {
       // Counter(?)  - TODO: finish
       // 0 0 0 | 0 0 0 0 0  
       //            State (29)
       unsigned char cell_state{};
       State get_state(){
            return (State)(cell_state & 0x1f);
       }
       void set_state(State s){
            cell_state &= 0xe0;
            cell_state |= s;
       }
       State next_state = U;

       static State next(State current, State up, State down, State left, State right){
            bool is_signal = (down == T01A || down == T11A || up == T03A || up == T13A || 
                        left == T00A || left == T10A || right == T02A || right == T12A);
            bool is_spec_signal = (down == T11A || up == T13A || left == T10A || right == T12A);  
            bool is_norm_signal = (down == T01A || up == T03A || left == T00A || right == T02A);     
                      
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
            if (std::any_of(std::begin(transport_normal), std::end(transport_normal), [current](auto el){return el == current;})){
                return is_norm_signal ? (State)(current + 4): (State)(is_spec_signal ? U : current);
            }
            if (std::any_of(std::begin(transport_normal_active), std::end(transport_normal_active), [current](auto el){return el == current;})){
                return is_norm_signal ? (State)(current): (State)(is_spec_signal ? U : current-4);
            }
            if (std::any_of(std::begin(transport_special), std::end(transport_special), [current](auto el){return el == current;})){
                return is_spec_signal ? (State)(current + 4): (State)(is_norm_signal ? U : current);
            }
            if (std::any_of(std::begin(transport_special_active), std::end(transport_special_active), [current](auto el){return el == current;})){
                return is_spec_signal ? (State)(current): (State)(is_norm_signal ? U : current-4);
            }
            return current;
       }
    };
    Cell field[WIDTH_N * HEIGHT_N]{};

    void next(){
        for (size_t i = 0; i < HEIGHT_N; ++i){
            for (size_t j = 0; j < WIDTH_N; ++j){
                auto& cell =  field[(i * WIDTH_N) + j];
                auto up_n   = field[((((i + HEIGHT_N - 1) % HEIGHT_N) * WIDTH_N) + j)].get_state();
                auto down_n = field[((((i + HEIGHT_N + 1) % HEIGHT_N) * WIDTH_N) + j)].get_state();
                auto left_n  = field[((i * WIDTH_N) + ((j + WIDTH_N - 1) % WIDTH_N))].get_state();
                auto right_n = field[((i * WIDTH_N) + ((j + WIDTH_N + 1) % WIDTH_N))].get_state();
                cell.next_state = Automata::Cell::next(cell.get_state(), up_n, down_n, left_n, right_n);
            }
        }

        for (size_t i = 0; i < HEIGHT_N; ++i){
            for (size_t j = 0; j < WIDTH_N; ++j)
                field[(i * WIDTH_N) + j].set_state(field[(i * WIDTH_N) + j].next_state);
        }
    }
};

void draw_figure_by_state(State s, Rectangle rec, float offset){
    if (std::any_of(std::begin(transport), std::end(transport), [s](auto el){return el == s;})) {
    Direction d;
    Color c;
    switch(s){
        case T13: c = BLACK;   d = DOWN;  break;
        case T03: c = BLUE;    d = DOWN;  break;
        case T12: c = BLACK;   d = LEFT;  break;
        case T02: c = BLUE;    d = LEFT;  break;
        case T10: c = BLACK;   d = RIGHT; break;
        case T00: c = BLUE;    d = RIGHT; break;
        case T11: c = BLACK;   d = UP;    break;
        case T01: c = BLUE;    d = UP;    break;
        
        case T13A: c = PURPLE; d = DOWN;  break;
        case T03A: c = GREEN;  d = DOWN;  break;
        case T12A: c = PURPLE; d = LEFT;  break;
        case T02A: c = GREEN;  d = LEFT;  break;
        case T10A: c = PURPLE; d = RIGHT; break;
        case T00A: c = GREEN;  d = RIGHT; break;
        case T11A: c = PURPLE; d = UP;    break;
        case T01A: c = GREEN;  d = UP;    break;
    }
    
    auto [a3,a2,a1] = figure_by_state(d, rec.width, rec.height);
    DrawTriangle({a3.x + rec.x + offset, a3.y + rec.y + offset}, {a2.x + rec.x + offset, a2.y + rec.y + offset}, {a1.x + rec.x + offset, a1.y + rec.y + offset}, c);
    }
}

struct SelectMenu {
    static constexpr float offset = 16;
    State state = U;
    bool active = false;
    Rectangle menu = {0,0,SELECT_MENU_W,SELECT_MENU_H};
    State arr[STATE_H][STATE_W]{
        {U},
        {S, S0, S00, S01, S000, S1, S10, S11},
        {C00, C10, C01, C11},
        {T00, T01, T02, T03},
        {T00A, T01A, T02A, T03A},
        {T10, T11, T12, T13},
        {T10A, T11A, T12A, T13A},
    };
    const char* arr_str[STATE_H][STATE_W]{
        {"U"},
        {"S", "S0", "S00", "S01", "S000", "S1", "S10", "S11"},
        {"C00", "C10", "C01", "C11"},
        {"T00", "T01", "T02", "T03"},
        {"T00A", "T01A", "T02A", "T03A"},
        {"T10", "T11", "T12", "T13"},
        {"T10A", "T11A", "T12A", "T13A"},
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
        float x_w = SELECT_MENU_W/STATE_W;
        float y_w = SELECT_MENU_H/STATE_H;
        Rectangle rec;
        for (size_t i = 0; i < STATE_H; ++i){
            for (size_t j = 0; j < STATE_W; ++j){
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
                for (size_t j = 0; j < WIDTH_N; ++j){
                    auto pos = (i * WIDTH_N) + j;
                    auto rec = Rectangle{float(j * FIELD_WIDTH), float(i * FIELD_HEIGHT), FIELD_WIDTH, FIELD_HEIGHT};
                    if (!sm.active && CheckCollisionPointRec(GetMousePosition(), rec) && IsMouseButtonPressed(0))
                        a.field[pos].set_state(sm.state);
                    draw_figure_by_state(a.field[pos].get_state(), rec, 20);
                }
            }
            if (IsKeyPressed(KEY_N)) a.next();
            sm.draw();
        EndDrawing();
    }
    CloseWindow();
}