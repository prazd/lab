#include <iostream>
#include <bitset>
#include <stack>
#include <sstream>
#include <algorithm>
#include <vector>
#include "raylib.h"
#include "Turtle.hpp"
#include "UI.hpp"
#include "Constants.hpp"

// TODO:
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


template <size_t width, size_t height, size_t x, size_t y>
struct LSystem {
    std::vector<std::pair<char, std::string>> rules;
    std::string lstr;
    std::string axiom_str;
    size_t iterations = 0;
    size_t t_pos = 0;

    std::stringstream ss;

    bool ready = false;
    Turtle2D<width,height,x,y> t;

    UserInput<10, 100, 200, 50> angle_input;
    Rectangle angle_button;

    UserInput<10, 300,200,50> axiom_input;
    Rectangle axiom_button;

    UserInput<10, 200, 200, 50> key_input;
    UserInput<10, 250, 200, 50> value_input;
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

    LSystem(): 
        angle_button{210, 100,100,50},
        add_rule_button{210, 250,100,50},
        clear_rules_button{310, 250,100,50},
        axiom_button{210, 300,100,50},
        add_iter_button{10, 350,100,50},
        sub_iter_button{110, 350,100,50},
        run_button{10, 400,100,50}{}

    void handle(){
        if (CheckCollisionPointRec(GetMousePosition(), add_rule_button) && IsMouseButtonPressed(0))
            rules.push_back(std::pair{key_input.s[0], value_input.s});
        if (CheckCollisionPointRec(GetMousePosition(), clear_rules_button) && IsMouseButtonPressed(0))
            rules.clear();
        if (CheckCollisionPointRec(GetMousePosition(), angle_button) && IsMouseButtonPressed(0))
            t.s_angle = std::stof(angle_input.s);
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
        bool in_progress = t_pos != e;
        if (in_progress){
            for (int j = 0; j < 10; ++j) {
                if (t_pos != e){
                    t.handle(lstr[t_pos++]);
            }
           }
        }
        if (in_progress) t.update_texture();

        ss << "Configuration:\n";
        ss << "Axiom:" << axiom_str << '\n';
        ss << "IterationsCount:" << iterations << '\n';
        ss << "Rules:\n";
        for (const auto& el: rules) {
            ss << el.first << " :" << el.second << '\n';
        }
        ss << "Angle:" << t.s_angle << '\n';
        ss << "TurtlePosX:" << t.state.vec.x << '\n';
        ss << "TurtlePosY:" << t.state.vec.y << '\n';
        auto s_ss = ss.str();
        DrawText(s_ss.c_str(), 500, 0, 15.f, BLACK);
        ss.str("");
    }
};

void sierpinski_triangle_seed(auto& l){
    l.rules.push_back(std::pair{'F', "F-G+F+G-F"});
    l.rules.push_back(std::pair{'G', "GG"});
    l.axiom_str = "F-G-G";
    l.t.s_angle = 120;
    l.iterations = 7;
}

void sierpinski_carpet_seed(auto& l){
    l.rules.push_back(std::pair{'F', "F+F-F-F-G+F+F+F-F"});
    l.rules.push_back(std::pair{'G', "GGG"});
    l.axiom_str = "F-G-G";
    l.t.s_angle = 90;
    l.iterations = 7;
}

void dragon_curve_seed(auto& l){
    l.rules.push_back(std::pair{'X', "X+YF+"});
    l.rules.push_back(std::pair{'Y', "-FX-Y"});
    l.axiom_str = "FX";
    l.t.s_angle = 90;
    l.iterations = 7;
}

void koch_snowflake_seed(auto& l) {
    l.rules.push_back(std::pair{'F', "F+F--F+F"});
    l.axiom_str = "F--F--F";
    l.t.s_angle = 60;
    l.iterations = 5;
}

int main(){
    InitWindow(constants::WIDTH, constants::HEIGHT, "LindenmayerSystem");
    Rectangle origin_view{500, 500, 400, 400};
    LSystem<constants::WIDTH, constants::HEIGHT, 200, 200> l;
    
    long x = 0;
    long y = 0;

    sierpinski_triangle_seed(l);

    while(!WindowShouldClose()){
        BeginDrawing();
                ClearBackground(WHITE);
                    DrawTexturePro(l.t.target, {float(x), float(y), (float)origin_view.width, (float)origin_view.height}, origin_view, {0, 0},0, WHITE);
                    DrawRectangleLinesEx(origin_view, 1.f, BLACK);
                    if (CheckCollisionPointRec(GetMousePosition(), origin_view) && IsMouseButtonDown(0)){
                        origin_view.x = GetMousePosition().x - origin_view.width/2;
                        origin_view.y = GetMousePosition().y - origin_view.height/2;
                    }
                    if (CheckCollisionPointRec(GetMousePosition(), origin_view) && IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_EQUAL) && origin_view.width < ((float)constants::WIDTH)/1.5f && origin_view.height < ((float)constants::HEIGHT)/1.5f){
                        origin_view.width += 50.f;
                        origin_view.height += 50.f;
                    }
                    if (CheckCollisionPointRec(GetMousePosition(), origin_view) && IsKeyPressed(KEY_MINUS) && origin_view.width > 200 && origin_view.height > 200){
                        origin_view.width -= 50.f;
                        origin_view.height -= 50.f;
                    }
                    if (IsKeyDown(KEY_LEFT))      --x;
                    if (IsKeyDown(KEY_RIGHT))     ++x;
                    if (IsKeyDown(KEY_DOWN))      ++y;
                    if (IsKeyDown(KEY_UP))        --y;
                    l.handle(); 
                DrawFPS(0,0);
        EndDrawing();
    }
    CloseWindow();
}