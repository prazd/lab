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
constexpr float FRAME_WIDTH = 40.f;
constexpr float FRAME_HEIGHT = 40.f;
constexpr float FRAME_SZ = 40.f;
constexpr auto pacman_img_path = "/Users/prazd/Desktop/books/cpp/cpp/raylib/pacman/img/pacman.png";
Texture2D atlas;

enum class Direction{null, left, right, up, down};

void load_sprites() {
    atlas = LoadTextureFromImage(LoadImage(pacman_img_path));
}

struct Animation {
    Direction direction = Direction::right;
    int first;
    int last;
    int cur;
    float speed;
    float duration;

    void update() {
        float dt = GetFrameTime();
        duration-=dt;
        if (duration <= 0.f){
            duration = speed;
            cur = cur == last ? first : ++cur;
        }
    }

    Rectangle animation_frame(int num_frames_per_row, float offset_x = 0.f, float offset_y = 0.f) {
        float x = float(cur % num_frames_per_row) * FRAME_WIDTH;
        float y = float(cur / num_frames_per_row) * FRAME_HEIGHT;
        return (Rectangle){
            .x = x + offset_x,
            .y = y + offset_y,
            .width =  ((direction == Direction::left || direction == Direction::up) ? -FRAME_WIDTH : FRAME_WIDTH),
            .height = FRAME_HEIGHT
        };
    }

    void set_direction(Direction dir) {
        direction = dir;
    }
};

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
    "####.######.##.###.#####...###",
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

template <char... sym>
constexpr size_t count() {
    size_t res{};
    for (auto&& row: map){
        res += std::count_if(std::begin(row), std::end(row), [](auto el){return ((el == sym) || ...);});
    }
    return res;
}

struct CoinsMap {
    std::array<std::pair<std::pair<float, float>, bool>, count<'.'>()> coins;
    CoinsMap() {
        float x = 0.f;
        float y = 0.f;
        size_t i{};
        for (auto&& row: map) {
            for(auto el: row) {
                if (el == '.') {
                    coins[i++] = {{x+20, y+20}, true};
                }
                x += OFFSET;
            }
            x = 0;
            y += OFFSET;
        }
    }
};


constexpr auto fill_valid_points() {
    std::array<std::pair<float, float>, count<'#'>()> res;
    float x = 0.f;
    float y = 0.f;
    size_t i{};
    for (auto&& row: map) {
        for(auto el: row) {
            if (el == '#' || el == '@') {
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
            if (el == '.') DrawRectangleLinesEx((Rectangle){x, y, OFFSET, OFFSET}, 1, BLUE);
            x += OFFSET;
        }
        x = 0.f;
        y += OFFSET;
    }
}

struct Player {
    Animation anim = {
        .first = 0,
        .last = 2,
        .cur = 2,
        .speed = 0.07f,
        .duration = 0.1f
    };
    Rectangle rect;
    Vector2 pos{200,61};
    Vector2 velocity{PLAYER_VEL_X, PLAYER_VEL_Y};
    
    Direction dir;
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
        rect = anim.animation_frame(10, 0, 22);
        if (next != Direction::null && !collision(next)) {
            dir = next;
            anim.set_direction(dir);
            next = Direction::null;
        }
        if (collision(dir)){
            dir = Direction::null;
        }
        switch(dir){
            case Direction::left:  { move_left(dt);  anim.update();  break;}
            case Direction::right: { move_right(dt); anim.update();  break;}
            case Direction::up:    { move_up(dt);    anim.update();  break;}
            case Direction::down:  { move_down(dt);  anim.update();  break;}
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

struct Monster: Player {
    Monster(){
        float x = 0.f;
        float y = 0.f;
        for (auto&& row: map){
            for (auto el: row){
                if(el == '@') { // spawn
                    pos.x = x;
                    pos.y = y;
                    break;
                }
                x += OFFSET;
            }
            x = 0;
            y += OFFSET;
        }
    }
};

int main() {
    InitWindow(WIDTH, HEIGHT, "Pacman");
    load_sprites();
    SetTargetFPS(60);
    size_t scores = 0;
    bool running = true;
    Player p;
    CoinsMap cm;
    auto target = LoadRenderTexture(WIDTH, HEIGHT);
     auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(BLACK);
            draw_map();
            auto dt = GetFrameTime();
            if (IsKeyPressed(KEY_W))
                p.update(Direction::up);
            if (IsKeyPressed(KEY_S))
                p.update(Direction::down);
            if (IsKeyPressed(KEY_A))
                p.update(Direction::left);
            if (IsKeyPressed(KEY_D))
                p.update(Direction::right);

            p.move(dt);
            DrawRectangleLinesEx((Rectangle){p.pos.x-R_PLAYER, p.pos.y-R_PLAYER, QUBE_SIDE, QUBE_SIDE}, 1, BLUE);
            switch(p.dir) {
                case Direction::up:
                case Direction::down:
                {
                    DrawTexturePro(atlas, p.rect, {p.pos.x-R_PLAYER, p.pos.y-R_PLAYER, FRAME_WIDTH, FRAME_HEIGHT}, {0, FRAME_SZ}, 90.f, WHITE);
                    break;
                }
                default:
                {
                    DrawTexturePro(atlas, p.rect, {p.pos.x-R_PLAYER, p.pos.y-R_PLAYER, FRAME_WIDTH, FRAME_HEIGHT}, {0, 0}, 0.f, WHITE);
                    break;
                }
            }

            for (auto& el: cm.coins) {
                if (std::get<1>(el)) {
                    auto& [coord, active] = el;         
                    if (CheckCollisionRecs({p.pos.x-R_PLAYER, p.pos.y-R_PLAYER, FRAME_WIDTH, FRAME_HEIGHT}, {coord.first, coord.second, 10, 10})){
                        active = false;
                        continue;
                    }
                    DrawTexturePro(atlas, {80, 22, FRAME_WIDTH, FRAME_HEIGHT}, {coord.first, coord.second, 10, 10}, {0, 0}, 0.f, WHITE);
                }
            }

            
            

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

    UnloadTexture(atlas);
    CloseWindow();
    return 0;
}