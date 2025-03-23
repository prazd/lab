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
constexpr float MONSTER_FRAME_HEIGHT = 40.f;
constexpr float MONSTER_FRAME_WIDTH = 35.f;
constexpr float FRAME_SZ = 40.f;
constexpr float COINS_OFFSET = 15.f;
constexpr float PLAYER_SPAWN_OFFSET_X = 21.f;
constexpr float PLAYER_SPAWN_OFFSET_Y = 21.f;
constexpr float MONSTER_VIEW_DISTANCE = 200.f;

//#define DRAW_LINE = 1
// #define DEBUG_LOGS = 1

constexpr auto pacman_img_path = "/Users/prazd/Desktop/books/cpp/cpp/raylib/pacman/img/pacman.png";
Texture2D atlas;

void load_sprites() {
    atlas = LoadTextureFromImage(LoadImage(pacman_img_path));
}

enum class Direction{left, right, up, down, null};

struct Animation {
    Direction direction = Direction::right;
    int first;
    int last;
    int cur;
    float speed;
    float duration;
    float width;
    float height;

    void update() {
        float dt = GetFrameTime();
        duration-=dt;
        if (duration <= 0.f){
            duration = speed;
            cur = cur == last ? first : ++cur;
        }
    }

    Rectangle animation_frame_adaptive(int num_frames_per_row, float offset_x = 0.f, float offset_y = 0.f) {
        float x = float(cur % num_frames_per_row) * width;
        float y = float(cur / num_frames_per_row) * height;
        return (Rectangle){
            .x = x + offset_x,
            .y = y + offset_y,
            .width =  ((direction == Direction::left || direction == Direction::up) ? -width : width),
            .height = height
        };
    }

    Rectangle animation_frame(int num_frames_per_row, float offset_x = 0.f, float offset_y = 0.f) {
        float x = float(cur % num_frames_per_row) * width;
        float y = float(cur / num_frames_per_row) * height;
        return (Rectangle){
            .x = x + offset_x,
            .y = y + offset_y,
            .width =  width,
            .height = height
        };
    }

    void set_direction(Direction dir) {
        direction = dir;
    }
};

template <typename T, int a = 0, int b = 0>
class Random{
    static inline std::random_device rd;
    static inline std::mt19937 gen{rd()};
    static inline std::uniform_int_distribution<T> dis = std::uniform_int_distribution<T>(a, b);
public:
    static int roll() {
        return dis(gen);
    }
    static int roll(int l, int r) {
        std::uniform_int_distribution<T> cdis = std::uniform_int_distribution<int>(l, r);
        return cdis(gen);
    }
};

constexpr std::array<std::string_view, 15> map = {
    "##############################",
    "####.......................###",
    "####.######.##.###.#######.###",
    "####.p#####.##.###.#####@..###",
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

constexpr auto fill_coords() {
    std::array<Vector2, 30 * 15> res{};
    float x = 0.f;
    float y = 0.f;
    size_t i{};
    for (auto&& raw: map) {
        for (auto el: raw) {
            res[i++] = {x, y};
            x += OFFSET;
        }
        x = 0;
        y += OFFSET;
    }
    return res;
}

constexpr std::array<Vector2, 30 * 15> coords = fill_coords();

template <typename Func>
constexpr void map_elements_call(Func&& f) {
    size_t i{};
    size_t coords_i{};
    for (auto&& row: map) {
        for(auto el: row) {
            f(i, coords_i, el);
            ++coords_i;
        }
    }
}

constexpr Vector2 get_point(char p) {
    Vector2 res{};
    map_elements_call([&res, p](auto, auto coord_i, auto el){
        if (el == p) {
            res = coords[coord_i];
        }
    });
    return res;
}

template <char... sym>
constexpr size_t count() {
    size_t res{};
    for (auto&& row: map){
        res += std::count_if(std::begin(row), std::end(row), [](auto el){return ((el == sym) || ...);});
    }
    return res;
}


struct CoinsMap {
    size_t collected = 0;
    static inline std::array<std::pair<std::pair<float, float>, bool>, count<'.'>()> coins;
    CoinsMap() {
        map_elements_call([](auto& i, auto coords_i, auto el){
            if (el == '.'){
                auto v = coords[coords_i];
                coins[i++] = {{v.x+COINS_OFFSET, v.y+COINS_OFFSET}, true};
            }
        });
    }

    bool collect_all() const {
        return collected == coins.size();
    }

    void reset() {
        collected = 0;
        for (auto& el: coins) {
            auto& [p, active] = el;
            active = true;
        }
    }
};


constexpr auto fill_walls() {
    std::array<Vector2, count<'#'>()> res;
    map_elements_call([&res](auto& i, auto& coords_i, auto el){
        if (el == '#') {
            res[i++] = coords[coords_i];
         }
    });
    return res;
}

constexpr auto valid_points = fill_walls();

void draw_map() {
    map_elements_call([](auto, auto coord_i, auto el) {
        auto [x,y] = coords[coord_i];
        if (el == '.' || el == 'p') DrawRectangleLinesEx((Rectangle){x, y, OFFSET, OFFSET}, 1, BLUE);
        else if (el == '@') DrawRectangleLinesEx((Rectangle){x, y, OFFSET, OFFSET}, 1, RED);
    });
}

struct Player {
    Animation anim = {
        .first = 0,
        .last = 2,
        .cur = 2,
        .speed = 0.07f,
        .duration = 0.1f,
        .width = FRAME_WIDTH,
        .height = FRAME_HEIGHT
    };
    Rectangle rect = anim.animation_frame_adaptive(10, 0, 22);
    Vector2 pos = get_point('p');
    Vector2 velocity{PLAYER_VEL_X, PLAYER_VEL_Y};

    Direction dir = Direction::null;
    Direction next = Direction::null;

    Player() {
        pos.x += PLAYER_SPAWN_OFFSET_X;
        pos.y += PLAYER_SPAWN_OFFSET_Y;
    }

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
#ifdef DRAW_LINE
        DrawRectangleLinesEx((Rectangle){current_x, current_y, QUBE_SIDE, QUBE_SIDE}, 1, GREEN);
#endif
        return std::any_of(std::begin(valid_points), std::end(valid_points), [current_x, current_y](auto& el){
            if (CheckCollisionRecs((Rectangle){current_x, current_y, QUBE_SIDE, QUBE_SIDE},
                                  (Rectangle){el.x, el.y, OFFSET, OFFSET})){
#ifdef DRAW_LINE
                DrawRectangleLinesEx((Rectangle){el.x, el.y, QUBE_SIDE, QUBE_SIDE}, 1, YELLOW);
#endif
            }
            return CheckCollisionRecs((Rectangle){current_x, current_y, QUBE_SIDE, QUBE_SIDE},
            (Rectangle){el.x, el.y, OFFSET, OFFSET});
        });
    }

    void update(Direction d) {
        next = d;
    }

    void move(float dt) {
        rect = anim.animation_frame_adaptive(10, 0, 22);
        if (next != Direction::null && !collision(next)) {
            dir = next;
            anim.set_direction(dir);
            next = Direction::null;
        }
        if (collision(dir)){
            dir = Direction::null;
            anim.cur = anim.last;
            rect = anim.animation_frame_adaptive(10, 0, 22);
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
    Rectangle ray;

    Monster(){
        map_elements_call([this](auto, auto coord_i, auto el) {
            if(el == '@') {
              auto [x, y] = coords[coord_i];
              pos = {x + 21, y + 21};
           }
      });

        anim = {
            .first = 0,
            .last = 1,
            .cur = 0,
            .speed = 0.07f,
            .duration = 0.1f,
            .width = MONSTER_FRAME_WIDTH,
            .height = MONSTER_FRAME_HEIGHT
        };

        rect = anim.animation_frame(10, 3, 80);
        dir = Direction::right;
        anim.set_direction(dir);
        
    }


    void move(float dt, const Player& p) {
        // left, right, up, down
        std::array<bool, 4> current{collision(Direction::left), collision(Direction::right), collision(Direction::up), collision(Direction::down)};
        
        if (dir != Direction::null) {
        #ifdef  DEBUG_LOGS
                    if (CheckCollisionRecs(ray, {p.pos.x-R_PLAYER, p.pos.y-R_PLAYER, FRAME_WIDTH, FRAME_HEIGHT})){
                        std::cout << "----------------------" << '\n';
                        std::cout << "PX:"    <<    p.pos.x-R_PLAYER << '\n';
                        std::cout << "PY:"    <<    p.pos.y-R_PLAYER << '\n';
                        std::cout << "RayX:"  <<    ray.x << '\n';
                        std::cout << "RayY:"  <<    ray.y << '\n';
                    }
        #endif
            if (!CheckCollisionRecs(ray, {p.pos.x-R_PLAYER, p.pos.y-R_PLAYER, FRAME_WIDTH, FRAME_HEIGHT})) {
                for (size_t i = 0; i < 4; ++i) {
                    if (!current[i]){
                        auto cd = static_cast<Direction>(i);
                        if (dir == cd) continue;
                        if (dir == Direction::left && cd == Direction::right ||
                            dir == Direction::up && cd == Direction::down ||
                            dir == Direction::right && cd == Direction::left ||
                            dir == Direction::down && cd == Direction::up
                            ) continue;
                        auto v = Random<int, 1,2>::roll();
                        if (v == 1) continue;
                        else dir = static_cast<Direction>(i);
                        break;
                    }
                }
            }   
        } else {
            auto leagal_paths = std::count(current.begin(), current.end(), false);
            auto r = Random<int>::roll(1, leagal_paths);
            size_t curr{};
            for (size_t i = 0; i < 4; ++i) {
                if (!current[i]) {
                    ++curr;
                    if (curr == r) {
                        dir = static_cast<Direction>(i);
                        anim.set_direction(dir);
                        break;
                    }
                }
            }
        }
#ifdef DRAW_LINE
        DrawRectangleLinesEx(ray, 1, GREEN);
#endif        

        if (collision(dir)){
            dir = Direction::null;
        }

        switch(dir){
            case Direction::left:  { move_left(dt);  anim.update();  rect = anim.animation_frame(10, 3, 80); break;}
            case Direction::right: { move_right(dt); anim.update();  rect = anim.animation_frame(10, 73, 80); break;}
            case Direction::up:    { move_up(dt);    anim.update();  rect = anim.animation_frame(10, 143, 80); break;}
            case Direction::down:  { move_down(dt);  anim.update();  rect = anim.animation_frame(10, 213, 80); break;}
            default: break;
        }
        update_ray();
     }

     void update_ray() {
        switch(dir) {
            case Direction::left:   {ray = {pos.x-MONSTER_VIEW_DISTANCE, pos.y, MONSTER_VIEW_DISTANCE, 1};break;}
            case Direction::right:  {ray = {pos.x, pos.y, MONSTER_VIEW_DISTANCE, 1};break;}
            case Direction::up:     {ray = {pos.x, pos.y-MONSTER_VIEW_DISTANCE, 1, MONSTER_VIEW_DISTANCE};break;}
            case Direction::down:   {ray = {pos.x, pos.y, 1, MONSTER_VIEW_DISTANCE};break;}
            default: break;
        }
     }
};



int main() {
    InitWindow(WIDTH, HEIGHT, "Pacman");
    load_sprites();
    SetTargetFPS(60);
    bool running = true;
    bool win = false;
    Player p;
    CoinsMap cm;
    std::array<Monster, 3> ms;

    std::stringstream score_text;

    auto monster_spawn_vec = get_point('@');

    auto stream_updater = [](std::stringstream& ss, auto&&... data) {
        ss.str("");
        (ss << ... << data);
    };

    auto restart = [&]{
        p = Player{};
        cm.reset();
        ms = std::array<Monster, 3>{};
        stream_updater(score_text, "Score:0");
        running = true;
        win = false;
    };

    auto target = LoadRenderTexture(WIDTH, HEIGHT);
     auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(BLACK);
            if (!running) {
                stream_updater(score_text, "Game Over. Scores ", cm.collected, ". Press R to restart");
                if (!win) DrawText(score_text.str().data(), WIDTH/4, HEIGHT/2, 30, WHITE);
                else DrawText("YOU WIN. Press R to restart", WIDTH/4, HEIGHT/2, 30, WHITE);

                EndTextureMode();
                if (IsKeyPressed(KEY_R)) {
                    restart();
                }
                return;
            }
            draw_map();
            auto dt = GetFrameTime();
            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
                p.update(Direction::up);
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
                p.update(Direction::down);
            if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))
                p.update(Direction::left);
            if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))
                p.update(Direction::right);

            p.move(dt);
#ifdef DRAW_LINE
            DrawRectangleLinesEx((Rectangle){p.pos.x-R_PLAYER, p.pos.y-R_PLAYER, QUBE_SIDE, QUBE_SIDE}, 1, BLUE);
#endif
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
                        ++cm.collected;
                        continue;
                    }
                    DrawTexturePro(atlas, {80, 22, FRAME_WIDTH, FRAME_HEIGHT}, {coord.first, coord.second, 10, 10}, {0, 0}, 0.f, WHITE);
                }
            }

            for (auto& m: ms) {
                m.move(dt, p);
                if (CheckCollisionRecs({p.pos.x-R_PLAYER, p.pos.y-R_PLAYER, FRAME_WIDTH, FRAME_HEIGHT}, {m.pos.x-R_PLAYER, m.pos.y-R_PLAYER, FRAME_WIDTH, FRAME_HEIGHT})){
                    running = false;
                    win = false;
                }
                DrawTexturePro(atlas, m.rect, {m.pos.x-R_PLAYER, m.pos.y-R_PLAYER, FRAME_WIDTH, FRAME_HEIGHT}, {2, 5}, 0.f, WHITE);
            }


            if (CheckCollisionRecs({p.pos.x-R_PLAYER, p.pos.y-R_PLAYER, FRAME_WIDTH, FRAME_HEIGHT}, {monster_spawn_vec.x, monster_spawn_vec.y, 10,10}) && cm.collected == cm.coins.size()){
                running = false;
                win = true;
            }

            stream_updater(score_text, "Score:", cm.collected);
            DrawText(score_text.str().data(), 0, 0, 20, WHITE);

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