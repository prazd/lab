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
    Turtle2D<width,height,x,y> t2d;
    Turtle3D<width,height,x,y,10> t3d;
    enum Version: unsigned char {T2D, T3D};
    Version version = T2D;

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


    UserInput<10, 450, 200, 50> result_str_input;
    Rectangle run_result_str_button;
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
        run_result_str_button{210, 450,100,50},
        run_button{10, 400,100,50}
        {}

    void turtle_update(){
        if (version == T2D)
            t2d.update_texture();
    }
    void turtle_handle(char s){
        if (version == T2D)
            t2d.handle(s);
        else 
            t3d.handle(s);
    }
    void turtle_angle_update(float angle){
        if (version == T2D)
            t2d.s_angle = angle;
        else
            t3d.s_angle = angle;
    }
    void turtle_clear(){
        if (version == T2D)
            t2d.clear_texture();
        else 
            t3d.clear_texture();
    }
    float get_turtle_angle(){
        if (version == T2D)
            return t2d.s_angle;
        else 
            return t3d.s_angle;
    }
    void turtle_draw(Rectangle origin_view, float x_pos, float y_pos){
        if (version == T2D){
            t2d.draw(origin_view, x_pos, y_pos);
        } else {
            t3d.draw(origin_view, x_pos, y_pos);
        }
    }

    void handle(Rectangle origin_view, float x_pos, float y_pos){
        if (CheckCollisionPointRec(GetMousePosition(), add_rule_button) && IsMouseButtonPressed(0))
            rules.push_back(std::pair{key_input.s[0], value_input.s});
        if (CheckCollisionPointRec(GetMousePosition(), clear_rules_button) && IsMouseButtonPressed(0))
            rules.clear();
        if (CheckCollisionPointRec(GetMousePosition(), angle_button) && IsMouseButtonPressed(0))
            turtle_angle_update(std::stof(angle_input.s));
        if (CheckCollisionPointRec(GetMousePosition(), axiom_button) && IsMouseButtonPressed(0))
            axiom_str = axiom_input.s;
        if (CheckCollisionPointRec(GetMousePosition(), add_iter_button) && IsMouseButtonPressed(0))
            ++iterations;
        if (CheckCollisionPointRec(GetMousePosition(), sub_iter_button) && IsMouseButtonPressed(0) && iterations)
            --iterations;
        if (CheckCollisionPointRec(GetMousePosition(), run_button) && IsMouseButtonPressed(0)){
            // t.clear_texture();
            turtle_clear();
            lstr = generate_cmds(axiom_str, rules, iterations);
            t_pos = 0;
        }
        if (CheckCollisionPointRec(GetMousePosition(), run_result_str_button) && IsMouseButtonPressed(0)){
            turtle_clear();
            lstr = result_str_input.s;
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

        result_str_input.handle();
        draw_button(run_result_str_button, "res str run", RED);

        size_t e = lstr.size();
        bool in_progress = t_pos != e;
        if (in_progress){
            for (int j = 0; j < 10; ++j) {
                if (t_pos != e){
                   turtle_handle(lstr[t_pos++]);
            }
           }
        }
        if (in_progress) turtle_update();

        ss << "Configuration:\n";
        ss << "Axiom:" << axiom_str << '\n';
        ss << "IterationsCount:" << iterations << '\n';
        ss << "Rules:\n";
        for (const auto& el: rules) {
            ss << el.first << " :" << el.second << '\n';
        }
        if (version == T2D){
            ss << "Angle:" << t2d.s_angle << '\n';
            ss << "TurtlePosX:" << t2d.state.vec.x << '\n';
            ss << "TurtlePosY:" << t2d.state.vec.y << '\n';
        } else {
            ss << "Angle:" << t3d.s_angle << '\n';
            ss << "TurtlePosX:" << t3d.state.pos.x << '\n';
            ss << "TurtlePosY:" << t3d.state.pos.y << '\n';
            ss << "TurtlePosZ:" << t3d.state.pos.z << '\n';
        }
       
        auto s_ss = ss.str();
        DrawText(s_ss.c_str(), 500, 0, 15.f, BLACK);
        ss.str("");
        turtle_draw(origin_view, x_pos, y_pos);
    }
};

void sierpinski_triangle_seed(auto& l){
    l.rules.push_back(std::pair{'F', "F-G+F+G-F"});
    l.rules.push_back(std::pair{'G', "GG"});
    l.axiom_str = "F-G-G";
    l.turtle_angle_update(120);
    l.iterations = 7;
}

void sierpinski_carpet_seed(auto& l){
    l.rules.push_back(std::pair{'F', "F+F-F-F-G+F+F+F-F"});
    l.rules.push_back(std::pair{'G', "GGG"});
    l.axiom_str = "F-G-G";
    l.turtle_angle_update(90);
    l.iterations = 7;
}

void dragon_curve_seed(auto& l){
    l.rules.push_back(std::pair{'X', "X+YF+"});
    l.rules.push_back(std::pair{'Y', "-FX-Y"});
    l.axiom_str = "FX";
    l.turtle_angle_update(90);
    l.iterations = 7;
}

void koch_snowflake_seed(auto& l) {
    l.rules.push_back(std::pair{'F', "F+F--F+F"});
    l.axiom_str = "F--F--F";
    l.turtle_angle_update(60);
    l.iterations = 5;
}

int main(){
    InitWindow(constants::WIDTH, constants::HEIGHT, "LindenmayerSystem");
    Rectangle origin_view{500, 500, 400, 400};
    LSystem<constants::WIDTH, constants::HEIGHT, 200, 200> l;
    
    float x = 0;
    float y = 0;

    sierpinski_triangle_seed(l);

    while(!WindowShouldClose()){
        BeginDrawing();
                ClearBackground(WHITE);
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
                    if (IsKeyDown(KEY_LEFT))      x-=1.f;
                    if (IsKeyDown(KEY_RIGHT))     x+=1.f;
                    if (IsKeyDown(KEY_DOWN))      y+=1.f;
                    if (IsKeyDown(KEY_UP))        y-=1.f;
                    if (IsKeyPressed(KEY_M))      {l.version = (decltype(l)::Version)(l.version ^ 1); l.turtle_clear();}
                    l.handle(origin_view, x, y); 
                DrawFPS(0,0);
        EndDrawing();
    }
    CloseWindow();
}