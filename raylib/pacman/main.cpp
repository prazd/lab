#include "raylib.h"
#include <array>
#include <iostream>
#include <random>
#include <sstream>

constexpr float WIDTH = 1200.f;
constexpr float HEIGHT = 600.f;
constexpr float OFFSET = 40.f;
constexpr float R_PLAYER = 20.f;
constexpr float P_CHECK_VIEW = 2.f;
constexpr float PLAYER_VEL_X = 100.f;
constexpr float PLAYER_VEL_Y = 100.f;
constexpr float QUBE_SIDE = OFFSET - P_CHECK_VIEW;

template <typename T, int a, int b>
class Random{
    static inline std::random_device rd;
    static inline std::mt19937 gen{rd()};
    static inline std::uniform_int_distribution<T> dis = std::uniform_int_distribution<T>(a,b);
public:
    static int roll() {
        return dis(gen);
    }
};

constexpr std::array<std::string_view, 15> map = {
    "##############################",
    "####.......................###",
    "####.######.##.###.#######.###",
    "####.######.##.###.#######.###",
    "####.######.##.###.#######.###",
    "####.######.##.###.#######.###",
    "####.######.##.###.#######.###",
    "####.###.##.##.###.##.####.###",
    "####.###.##.##.###.##.####.###",
    "####.###.##.##.###.##.####.###",
    "####.###.##.##.###.##.####.###",
    "####.###.##.##.###.##.####.###",
    "####.###.##.##.###.##.####.###",
    "####.......................###",
    "##############################"
};

constexpr size_t count_dots() {
    size_t res{};
    for (auto&& row: map){
        res += std::count_if(std::begin(row), std::end(row), [](auto el){return el == '#';});
    }
    return res;
}


constexpr auto fill_valid_points() {
    std::array<std::pair<float, float>, count_dots()> res;
    float x = 0.f;
    float y = 0.f;
    size_t i{};
    for (auto&& row: map) {
        for(auto el: row) {
            if (el == '#') {
               res[i++] = {x, y};
            }
            x += OFFSET;
        }
        x = 0;
        y += OFFSET;
    }
    return res;
}

constexpr auto valid_points = fill_valid_points();

void draw_map() {
    float x = 0.f;
    float y = 0.f;
    for (auto&& row: map){
        for (auto el: row) {
            if (el == '#') {
                DrawRectangleRec((Rectangle){x, y, OFFSET, OFFSET}, BLACK);
            } else {
                DrawRectangleRec((Rectangle){x, y, OFFSET, OFFSET}, WHITE);
            }
            x += OFFSET;
        }
        x = 0.f;
        y += OFFSET;
    }
}

struct Player {
    Vector2 pos{200,61};
    Vector2 velocity{PLAYER_VEL_X, PLAYER_VEL_Y};
    enum class Direction{null, left, right, up, down} dir = Direction::null;
    Direction next = Direction::null;

    bool collision(Direction d) {
        auto current_x = pos.x - R_PLAYER;
        auto current_y = pos.y - R_PLAYER;
        switch(d) {
            case Direction::left:  {current_x -= P_CHECK_VIEW; break;}
            case Direction::right: {current_x += P_CHECK_VIEW; break;}
            case Direction::up:    {current_y -= P_CHECK_VIEW; break;}
            case Direction::down:  {current_y += P_CHECK_VIEW; break;}
            default: return true;
        }
        DrawRectangleLinesEx((Rectangle){current_x, current_y, QUBE_SIDE, QUBE_SIDE}, 1, GREEN);
        return std::any_of(std::begin(valid_points), std::end(valid_points), [current_x, current_y](auto& el){
            if (CheckCollisionRecs((Rectangle){current_x, current_y, QUBE_SIDE, QUBE_SIDE},
                                  (Rectangle){el.first, el.second, OFFSET, OFFSET})){
                DrawRectangleLinesEx((Rectangle){el.first, el.second, QUBE_SIDE, QUBE_SIDE}, 1, YELLOW);
            }
            return CheckCollisionRecs((Rectangle){current_x, current_y, QUBE_SIDE, QUBE_SIDE},
            (Rectangle){el.first, el.second, OFFSET, OFFSET});
        });
    }
    
    void update(Direction d) {
        next = d;
    }

    void move(float dt) {
        if (next != Direction::null && !collision(next)) {
            dir = next;
            next = Direction::null;
        }
        if (collision(dir)){
            dir = Direction::null;
        }
        switch(dir){
            case Direction::left:  { move_left(dt);  break;}
            case Direction::right: { move_right(dt); break;}
            case Direction::up:    { move_up(dt);    break;}
            case Direction::down:  { move_down(dt);  break;}
            default: break;
        }
    }

    void move_up(float dt) {
        pos.y -= velocity.y * dt;
    }
    
    void move_down(float dt) {
        pos.y += velocity.y * dt;
    }

    void move_left(float dt) {
        pos.x -= velocity.x * dt;
    }

    void move_right(float dt){
        pos.x += velocity.x * dt;
    }
};

int main() {
    InitWindow(WIDTH, HEIGHT, "Pacman");
    SetTargetFPS(60);
    size_t scores = 0;
    bool running = true;
    Player p;
    auto target = LoadRenderTexture(WIDTH, HEIGHT);
     auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);
            draw_map();
            auto dt = GetFrameTime();
            if (IsKeyPressed(KEY_W))
                p.update(Player::Direction::up);
            if (IsKeyPressed(KEY_S))
                p.update(Player::Direction::down);
            if (IsKeyPressed(KEY_A))
                p.update(Player::Direction::left);
            if (IsKeyPressed(KEY_D))
                p.update(Player::Direction::right);
            p.move(dt);
            DrawCircle(p.pos.x, p.pos.y, R_PLAYER, RED);
            DrawRectangleLinesEx((Rectangle){p.pos.x-R_PLAYER, p.pos.y-R_PLAYER, QUBE_SIDE, QUBE_SIDE}, 1, BLUE);
        EndTextureMode();
    };

    while(!WindowShouldClose()){
        render();
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height}, {0, 0}, WHITE);
            DrawFPS(1000, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}