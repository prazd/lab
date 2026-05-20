#ifndef TURTLE_H
#define TURTLE_H
#include <array>
#include <stack>
#include "raylib.h"
#include "raymath.h"



template <size_t width, size_t height, size_t x, size_t y>
struct Turtle2D {
    float step = 1.f;
    float s_angle = 120;
    Texture2D target;
    static inline std::array<Color, width * height> buffer; 

    enum Commands: char {ForwardDraw = 'F', Forward = 'f', Plus = '+', Minus = '-', Push = '[', Pop = ']', GDraw = 'G'};
    struct State {
        Vector2 vec{};
        float angle{0};
    } state;
    std::stack<State> s;

    Turtle2D(): state({Vector2{x,y}}) {
        Image img = GenImageColor(width, height, WHITE);
        target = LoadTextureFromImage(img);
        UnloadImage(img);
        clear_texture();
    }

    void update_texture(){
        UpdateTexture(target, buffer.data());
    }

    void clear_texture(){
        std::fill(std::begin(buffer), std::end(buffer), WHITE);
        update_texture();
    }

    void draw_line(){
        auto next = Vector2{state.vec.x + step * std::cos((PI * state.angle)/180), state.vec.y + step * std::sin((PI * state.angle)/180)};
        size_t current_x = state.vec.x;
        size_t current_y = state.vec.y;
        if (current_x < width && current_y < height)
            buffer[(current_y * width) + current_x] = BLACK;
        state.vec = next;
    }
    void handle(char sym){
        switch(sym) {
            case GDraw:
            case ForwardDraw: {draw_line(); break;}
            case Forward:     {state.vec = Vector2{state.vec.x - step * std::cos((PI * state.angle)/180), state.vec.y - step * std::sin((state.angle * PI)/180)}; break;}
            case Plus:        {state.angle+=s_angle; break;}
            case Minus:       {state.angle-=s_angle; break;}
            case Push:        {s.push(state); break;}
            case Pop:         {state = s.top(); s.pop(); break;}
        }
    }
    void draw(Rectangle origin_view, float x_pos, float y_pos){
        DrawTexturePro(target, {x_pos, y_pos, (float)origin_view.width, (float)origin_view.height}, origin_view, {0, 0},0, WHITE); 
        DrawRectangleLinesEx(origin_view, 1.f, BLACK);
    }
};


template <size_t width, size_t height, size_t x, size_t y, size_t z>
struct Turtle3D {
    float step = 1.f;
    float s_angle = 120;

    RenderTexture2D rt;
    Camera3D camera;

    enum Commands: char {ForwardDraw = 'F', Forward = 'f', YawLeft = '+', YawRight = '-', PitchDown='&', PitchUp='^',RollLeft='/',RollRight='\'',
                        Push = '[', Pop = ']', GDraw = 'G'
                        };
    struct State {
        Vector3 pos{100,100,0};
        Vector3 h{};
        Vector3 l{};
        Vector3 u{};
        float angle{0};
    } state;
    
    std::stack<State> s;
    
    Turtle3D() {
        state.h = {0,0,1};
        state.l = {-1,0,0};
        state.u = {0,1,0};
        camera.position = {120,120,10};
        camera.target = {100,100,0};
        camera.up = {0,1,0};
        camera.fovy = 45.f;
        camera.projection = CAMERA_PERSPECTIVE;
        rt = LoadRenderTexture(width, height);
    }

    void clear_texture(){
          BeginTextureMode(rt);
          ClearBackground(WHITE);
          EndTextureMode();
    }

    void draw_line(){
        auto next = Vector3{
            state.pos.x + step * state.h.x, 
            state.pos.y + step * state.h.y, 
            state.pos.z + step * state.h.z
        };
        BeginTextureMode(rt);
          BeginMode3D(camera);
               DrawLine3D(state.pos, next, BLACK);
            EndMode3D();
        EndTextureMode();            
        state.pos = next;
    }

    float to_radians(float angle){
        return (PI * angle)/180;
    }

    void normalize(){
        state.h = Vector3Normalize(state.h);
        state.l = Vector3Normalize(Vector3CrossProduct(state.u, state.h));
        state.u = Vector3Normalize(Vector3CrossProduct(state.h, state.l));
    }

    Vector3 v_sum(Vector3 v1, Vector3 v2, float a, bool sub) {
        a = to_radians(a);
        return { v1.x * std::cos(a) + (v2.x * std::sin(a) * (sub ? -1 : 1)), 
                 v1.y * std::cos(a) + (v2.y * std::sin(a) * (sub ? -1 : 1)),
                 v1.z * std::cos(a) + (v2.z * std::sin(a) * (sub ? -1 : 1))};
    }
    void pitch(bool down){
        normalize();
        auto h = state.h;
        state.h = v_sum(state.h, state.u, s_angle, down);
        state.u = v_sum(state.u, h, s_angle, !down);
        
    }
    void roll(bool right){
        normalize();
        auto l = state.l;
        state.l = v_sum(state.l, state.u, s_angle, right);
        state.u = v_sum(state.u, l, s_angle, !right);
    }
    void yaw(bool right){
        normalize();
        auto h = state.h;
        state.h = v_sum(state.h, state.l, s_angle, right);
        state.l = v_sum(state.l, h, s_angle, !right);
    }

    void handle(char sym){
        switch(sym) {
            case GDraw:
            case ForwardDraw: {draw_line(); break;}
            case YawLeft:     {yaw(false); break;}
            case YawRight:    {yaw(true);  break;}
            case RollLeft:    {roll(false); break;}
            case RollRight:   {roll(true); break;}
            case PitchUp:     {pitch(false); break;}
            case PitchDown:   {pitch(true); break;}
            case Push:        {s.push(state); break;}
            case Pop:         {state = s.top(); s.pop(); break;}
        }
    }

    void draw(Rectangle origin_view, float x_pos, float y_pos){
        DrawTextureRec(rt.texture, origin_view, {origin_view.x,origin_view.y},WHITE);
        DrawRectangleLinesEx(origin_view, 1.f, BLACK);
    }
};

#endif