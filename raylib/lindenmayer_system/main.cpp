#include <iostream>
#include <bitset>
#include <stack>
#include <sstream>
#include "raylib.h"

constexpr float HEIGHT = 1000;
constexpr float WIDTH = 1500;

struct Turtle {
    float step = 1.f;
    float s_angle = 120;
    RenderTexture2D texture;

    enum Commands: char {ForwardDraw = 'F', Forward = 'f', Plus = '+', Minus = '-', Push = '[', Pop = ']', GDraw = 'G'};
    struct State {
        Vector2 vec{};
        float angle{0};
    } state;
    std::stack<State> s;
    
    Turtle(float width, float height, float x, float y): texture(LoadRenderTexture(width, height)), state({Vector2{x,y}}) {clear_texture();}
    void clear_texture(){
        BeginTextureMode(texture);
            ClearBackground(WHITE);
        EndTextureMode();
    }
    void handle(char sym){
        switch(sym) {
            case GDraw:
            case ForwardDraw: {
                BeginTextureMode(texture);
                    auto next = Vector2{state.vec.x + step * std::cos((PI * state.angle)/180), state.vec.y + step * std::sin((PI * state.angle)/180)};
                    DrawLineEx(state.vec, next, 1.f, BLACK);
                    state.vec = next;
                EndTextureMode();
                break;}
            case Forward: {state.vec = Vector2{state.vec.x - step * std::cos((PI * state.angle)/180), state.vec.y - step * std::sin((state.angle * PI)/180)}; break;}
            case Plus:    {state.angle+=s_angle; break;}
            case Minus:   {state.angle-=s_angle; break;}
            case Push:    {s.push(state); break;}
            case Pop:     {state = s.top(); s.pop(); break;}
        }
    }
    ~Turtle(){UnloadRenderTexture(texture);}
};

// f+f++f+f
// f
// f+f++f+f
// f+f++f+f + f+f++f+f ++ f+f++f+f + f+f++f+f
consteval size_t calc_command_count(char s, std::string_view command, size_t iterations){
    size_t s_count = std::count(std::begin(command), std::end(command), s);
    size_t non_s_count = command.size() - s_count;
    size_t pow_s_count = 1;
    size_t non_s_acc = 0;
    for (size_t i = 0; i < iterations; ++i) {
        non_s_acc += (pow_s_count * non_s_count);
        pow_s_count *= s_count;
    }
    return pow_s_count + non_s_acc;
}


struct UserInput {
    std::stringstream ss;
    std::string s;
    Rectangle rec;
    UserInput(float x, float y, float w, float h): rec{x, y, w, h} {}
    void push_handler() {
        if (!CheckCollisionPointRec(GetMousePosition(), rec)) return;
        auto c = GetCharPressed();
        if (!c) return;
        s.push_back((char)c);
    }
    void pop_handler() {
        if (!CheckCollisionPointRec(GetMousePosition(), rec)) return;
        if (IsKeyPressed(KEY_BACKSPACE) && !s.empty())
            s.pop_back();
    }
    void handle() {
        push_handler();
        pop_handler();
        DrawRectangleLinesEx({rec.x, rec.y, rec.width, rec.height}, 1.f, BLACK);
        ss << std::hex << s;
        if (CheckCollisionPointRec(GetMousePosition(), rec))
            ss << " |";
        auto r_str = ss.str();
        DrawText(r_str.data(), rec.x, rec.y, 15.f, BLACK);
        ss.str("");
    }
};

struct LSystem {
    std::vector<std::pair<char, std::string>> rules;
    std::string lstr;
    std::string axiom_str;
    size_t iterations = 0;
    size_t t_pos = 0;

    std::stringstream ss;

    bool ready = false;
    Turtle t;
    UserInput angle_input;
    Rectangle angle_button;

    UserInput axiom_input;
    Rectangle axiom_button;

    UserInput key_input;
    UserInput value_input;
    Rectangle add_rule_button;
    Rectangle clear_rules_button;

    Rectangle add_iter_button;
    Rectangle sub_iter_button;

    Rectangle run_button;

    static std::string generate_cmds(std::string axiom, const std::vector<std::pair<char, std::string>>& map, size_t iterations){
        std::string res = axiom;
        std::string tmp;
        bool found = false;
        for (size_t i = 0; i < iterations; ++i){
            tmp.clear();
            for (auto c: res) {
                for (auto m_el: map){
                    if (c == m_el.first) {
                        tmp += m_el.second;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    tmp += c;
                }
                found = false;
            }
            res = std::move(tmp);
        }
        return res;
    }

    void draw_button(Rectangle rec, const char* text, Color c, float sz = 12.f){
        DrawRectangleLinesEx(rec, 1.f, BLACK);
        DrawText(text, rec.x, rec.y, sz, c);
    }

    LSystem(float width, float height, float x, float y): 
        t(width, height, x, y), 
        angle_input{0, 100, 200, 50}, 
        angle_button{200, 100,100,50},

        key_input{0, 200, 200, 50}, 
        value_input{0, 250, 200, 50}, 
        add_rule_button{200, 250,100,50},
        clear_rules_button{300, 250,100,50},

        axiom_input{0, 300,200,50},
        axiom_button{200, 300,100,50},

        add_iter_button{0, 350,100,50},
        sub_iter_button{100, 350,100,50},
        run_button{0, 400,100,50}{}

    void handle(){
        if (CheckCollisionPointRec(GetMousePosition(), add_rule_button) && IsMouseButtonPressed(0))
            rules.push_back(std::pair{key_input.s[0], value_input.s});
        if (CheckCollisionPointRec(GetMousePosition(), clear_rules_button) && IsMouseButtonPressed(0))
            rules.clear();
        if (CheckCollisionPointRec(GetMousePosition(), angle_button) && IsMouseButtonPressed(0))
            t.s_angle = (float)std::stoul(angle_input.s, nullptr, 10);
        if (CheckCollisionPointRec(GetMousePosition(), axiom_button) && IsMouseButtonPressed(0))
            axiom_str = axiom_input.s;
        if (CheckCollisionPointRec(GetMousePosition(), add_iter_button) && IsMouseButtonPressed(0))
            ++iterations;
        if (CheckCollisionPointRec(GetMousePosition(), sub_iter_button) && IsMouseButtonPressed(0) && iterations)
            --iterations;
        if (CheckCollisionPointRec(GetMousePosition(), run_button) && IsMouseButtonPressed(0)){
            t.clear_texture();
            lstr = generate_cmds(axiom_str, rules, iterations);
            t_pos = 0;
        }
    
        value_input.handle();
        key_input.handle();
        draw_button(add_rule_button, "add rule", RED);
        draw_button(clear_rules_button, "clear rules", RED);


        draw_button(axiom_button, "set axiom", RED);
        axiom_input.handle();

        draw_button(angle_button, "update angle", RED);
        angle_input.handle();

        draw_button(add_iter_button, "add iter", RED);
        draw_button(sub_iter_button, "sub iter", RED);
        draw_button(run_button, "run_button", RED);

        size_t e = lstr.size();
        if (t_pos != e){
            for (int j = 0; j < 30; ++j) {
                if (t_pos != e){
                    t.handle(lstr[t_pos++]);
            }
           }
        }

        ss << "Configuration:\n";
        ss << "Axiom:" << axiom_str << '\n';
        ss << "IterationsCount:" << iterations << '\n';
        ss << "Rules:\n";
        for (const auto& el: rules) {
            ss << el.first << " :" << el.second << '\n';
        }
        ss << "Angle:" << t.s_angle << '\n';
        auto s_ss = ss.str();
        DrawText(s_ss.c_str(), 500, 0, 15.f, BLACK);
        ss.str("");
    }
};


int main(){
    InitWindow(WIDTH, HEIGHT, "LindenmayerSystem");
    // auto s = generate_cmds("F-G-G", std::initializer_list<std::pair<char, std::string_view>>{{'F', "F-G+F+G-F"}, {'G', "GG"}}, 8);
    LSystem l(WIDTH, HEIGHT, WIDTH/2, HEIGHT/2);
    while(!WindowShouldClose()){
        BeginDrawing();
                ClearBackground(WHITE);
                    DrawTextureRec(l.t.texture.texture, {0, 0, (float)l.t.texture.texture.width, -(float)l.t.texture.texture.height}, {0, 0}, WHITE);
                    l.handle();
                DrawFPS(0,0);
        EndDrawing();
    }
    CloseWindow();
}