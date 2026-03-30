#include <iostream>
#include "raylib.h"
#include <sstream>

constexpr size_t WIDTH = 800;
constexpr size_t HEIGHT = 800;


struct NandNode {
    static bool nand(bool a, bool b){
       return !(a & b);
    }
    static inline constexpr float CONNECT_SQUARE_SZ = 55;

    Rectangle rec{WIDTH/2,HEIGHT/2,200,100};
    Vector2 input1{rec.x-50, rec.y};
    Vector2 input2{rec.x-50, rec.y + rec.height};
    Rectangle i1_rec{input1.x,input1.y - CONNECT_SQUARE_SZ/2, CONNECT_SQUARE_SZ, CONNECT_SQUARE_SZ};
    Rectangle i2_rec{input2.x, input2.y - CONNECT_SQUARE_SZ/2, CONNECT_SQUARE_SZ, CONNECT_SQUARE_SZ};

    
    Vector2 output{rec.x + rec.width, rec.y + rec.height/2};
    Rectangle o_rec{output.x, output.y-CONNECT_SQUARE_SZ/2, CONNECT_SQUARE_SZ, CONNECT_SQUARE_SZ};
    bool a = false;
    bool b = false;
    bool out = false;
    bool connected_a = false;
    bool connected_b = false;
    bool show_recs = false;
    bool show_text = false;
    void move(){
        if (auto mp = GetMousePosition(); CheckCollisionPointRec(mp, rec)){
            if (IsMouseButtonDown(0)) {
                rec.x = mp.x - rec.width/2;
                rec.y = mp.y - rec.height/2;
            } else if (IsKeyPressed(KEY_F)) {
                a = !a;
            } else if (IsKeyPressed(KEY_S)) {
                b = !b;
            } else if (IsKeyPressed(KEY_W)) {
                show_recs = !show_recs;
            } else if (IsKeyPressed(KEY_T)) {
                show_text = !show_text;
            }
        } else if (auto mm = GetMouseWheelMove(); mm != 0){
            rec.width += mm > 0 ?   (rec.width/5) :  -(rec.width/5);
            rec.height += mm > 0 ?  (rec.height/5) : -(rec.height/5);
        }
    }
    void handle(){
        move();
        input1 = {rec.x-50, rec.y};
        input2 = {rec.x-50, rec.y + rec.height};
        i1_rec = {input1.x, input1.y - CONNECT_SQUARE_SZ/2, CONNECT_SQUARE_SZ, CONNECT_SQUARE_SZ};
        i2_rec = {input2.x, input2.y - CONNECT_SQUARE_SZ/2, CONNECT_SQUARE_SZ, CONNECT_SQUARE_SZ};
        o_rec  = {output.x, output.y-CONNECT_SQUARE_SZ/2, CONNECT_SQUARE_SZ, CONNECT_SQUARE_SZ};
        output = {rec.x + rec.width, rec.y + rec.height/2};
        out = nand(a, b);
        DrawRectangleLinesEx(rec, 2.f, BLACK);
        DrawLine(input1.x, input1.y, input1.x + 50, input1.y, a ?   GREEN : RED);
        DrawLine(input2.x, input2.y, input2.x + 50, input2.y, b ?   GREEN : RED);
        DrawLine(output.x, output.y, output.x + 50, output.y, out ? GREEN : RED);
        if (show_recs) {DrawRectangleLinesEx(i1_rec, 2.f, BLUE); DrawRectangleLinesEx(i2_rec, 2.f, BLUE);  DrawRectangleLinesEx(o_rec, 2.f, BLUE);}
        if (show_text){   std::stringstream ss;
                ss << "\n\n  A:" << a << " B:" << b << " O:" << out;
                auto s = ss.str();
                DrawText(s.c_str(), rec.x, rec.y, 12.f, BLACK);}
    }
};

struct Field {
    std::vector<NandNode> vec;
    void handle(){
        if (IsKeyPressed(KEY_ONE))
            vec.push_back(NandNode{});
        for (int i = 0, e = vec.size(); i < e; ++i){
            for (int j = 0; j < e; ++j) {
                if (i == j) continue;
                if (CheckCollisionRecs(vec[i].i1_rec, vec[j].o_rec)){
                    vec[i].connected_a = true;
                    vec[i].a = vec[j].out;
                }
                if (CheckCollisionRecs(vec[i].i2_rec, vec[j].o_rec)){
                    vec[i].connected_b = true;
                    vec[i].b = vec[j].out;
                }
            }
        }
        for (auto&& el : vec) el.handle();
    }
};

int main(){
    InitWindow(WIDTH, HEIGHT, "FC");
    Field f;
    SetTargetFPS(60);
    while(!WindowShouldClose()){
        BeginDrawing();
            ClearBackground(WHITE);
            DrawFPS(0,0);  
            f.handle();
        EndDrawing();
    }
    CloseWindow();
}