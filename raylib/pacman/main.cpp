#include "raylib.h"
#include <array>
#include <sstream>
#include "Animation.h"
#include "Random.h"
#include "Map.h"
#include "Constants.h"

//#define DRAW_LINE = 1
// #define DEBUG_LOGS = 1

constexpr auto pacman_img_path = "/Users/prazd/Desktop/books/cpp/cpp/raylib/pacman/img/pacman.png";
Texture2D atlas;

void load_sprites() {
    atlas = LoadTextureFromImage(LoadImage(pacman_img_path));
}

constexpr std::array<Vector2, 30 * 15> fill_coords() {
    std::array<Vector2, 30 * 15> res{};
    float x = 0.f;
    float y = 0.f;
    size_t i{};
    for (auto&& raw: Map::raw) {
        for (auto el: raw) {
            res[i++] = {x, y};
            x += gc::OFFSET;
        }
        x = 0;
        y += gc::OFFSET;
    }
    return res;
}

constexpr std::array<Vector2, 30 * 15> coords = fill_coords();

constexpr Vector2 get_point(char p) {
    Vector2 res{};
    Map::call([&res, p](auto, auto coord_i, auto el){
        if (el == p) {
            res = coords[coord_i];
        }
    });
    return res;
}


struct CoinsMap {
    size_t collected = 0;
    static inline std::array<std::pair<std::pair<float, float>, bool>, Map::count<'.'>()> coins;
    CoinsMap() {
        Map::call([](auto& i, auto coords_i, auto el){
            if (el == '.'){
                auto v = coords[coords_i];
                coins[i++] = {{v.x+gc::COINS_OFFSET, v.y+gc::COINS_OFFSET}, true};
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
    std::array<Vector2, Map::count<'#'>()> res;
    Map::call([&res](auto& i, auto& coords_i, auto el){
        if (el == '#') {
            res[i++] = coords[coords_i];
         }
    });
    return res;
}

constexpr auto valid_points = fill_walls();

void draw_map() {
    Map::call([](auto, auto coord_i, auto el) {
        auto [x,y] = coords[coord_i];
        if (el == '.' || el == 'p') DrawRectangleLinesEx((Rectangle){x, y, gc::OFFSET, gc::OFFSET}, 1, BLUE);
        else if (el == '@') DrawRectangleLinesEx((Rectangle){x, y, gc::OFFSET, gc::OFFSET}, 1, RED);
    });
}

struct Player {
    Animation anim = {
        .first = 0,
        .last = 2,
        .cur = 2,
        .speed = 0.07f,
        .duration = 0.1f,
        .width = gc::FRAME_WIDTH,
        .height = gc::FRAME_HEIGHT
    };
    Rectangle rect = anim.animation_frame_adaptive(10, 0, 22);
    Vector2 pos = get_point('p');
    Vector2 velocity{gc::PLAYER_VEL_X, gc::PLAYER_VEL_Y};

    Direction dir = Direction::null;
    Direction next = Direction::null;

    Player() {
        pos.x += gc::PLAYER_SPAWN_OFFSET_X;
        pos.y += gc::PLAYER_SPAWN_OFFSET_Y;
    }

    bool collision(Direction d) {
        auto current_x = pos.x - gc::R_PLAYER;
        auto current_y = pos.y - gc::R_PLAYER;
        switch(d) {
                case Direction::left:  {current_x -= gc::P_CHECK_VIEW; break;}
                case Direction::right: {current_x += gc::P_CHECK_VIEW; break;}
                case Direction::up:    {current_y -= gc::P_CHECK_VIEW; break;}
                case Direction::down:  {current_y += gc::P_CHECK_VIEW; break;}
            default: return true;
        }
#ifdef DRAW_LINE
        DrawRectangleLinesEx((Rectangle){current_x, current_y, QUBE_SIDE, QUBE_SIDE}, 1, GREEN);
#endif
        return std::any_of(std::begin(valid_points), std::end(valid_points), [current_x, current_y](auto& el){
            if (CheckCollisionRecs((Rectangle){current_x, current_y, gc::QUBE_SIDE, gc::QUBE_SIDE},
                                  (Rectangle){el.x, el.y, gc::OFFSET, gc::OFFSET})){
#ifdef DRAW_LINE
                DrawRectangleLinesEx((Rectangle){el.x, el.y, QUBE_SIDE, QUBE_SIDE}, 1, YELLOW);
#endif
            }
            return CheckCollisionRecs((Rectangle){current_x, current_y, gc::QUBE_SIDE, gc::QUBE_SIDE},
            (Rectangle){el.x, el.y, gc::OFFSET, gc::OFFSET});
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
        Map::call([this](auto, auto coord_i, auto el) {
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
            .width = gc::MONSTER_FRAME_WIDTH,
            .height = gc::MONSTER_FRAME_HEIGHT
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
            if (!CheckCollisionRecs(ray, {p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT})) {
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
                    case Direction::left:   {ray = {pos.x-gc::MONSTER_VIEW_DISTANCE, pos.y, gc::MONSTER_VIEW_DISTANCE, 1};break;}
                    case Direction::right:  {ray = {pos.x, pos.y, gc::MONSTER_VIEW_DISTANCE, 1};break;}
                    case Direction::up:     {ray = {pos.x, pos.y-gc::MONSTER_VIEW_DISTANCE, 1, gc::MONSTER_VIEW_DISTANCE};break;}
                    case Direction::down:   {ray = {pos.x, pos.y, 1, gc::MONSTER_VIEW_DISTANCE};break;}
            default: break;
        }
     }
};



int main() {
    InitWindow(gc::WIDTH, gc::HEIGHT, "Pacman");
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

    auto target = LoadRenderTexture(gc::WIDTH, gc::HEIGHT);
     auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(BLACK);
            if (!running) {
                stream_updater(score_text, "Game Over. Scores ", cm.collected, ". Press R to restart");
                if (!win) DrawText(score_text.str().data(), gc::WIDTH/4, gc::HEIGHT/2, 30, WHITE);
                else DrawText("YOU WIN. Press R to restart", gc::WIDTH/4, gc::HEIGHT/2, 30, WHITE);

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
                    DrawTexturePro(atlas, p.rect, {p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {0, gc::FRAME_SZ}, 90.f, WHITE);
                    break;
                }
                default:
                {
                    DrawTexturePro(atlas, p.rect, {p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {0, 0}, 0.f, WHITE);
                    break;
                }
            }

            for (auto& el: cm.coins) {
                if (std::get<1>(el)) {
                    auto& [coord, active] = el;         
                    if (CheckCollisionRecs({p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {coord.first, coord.second, 10, 10})){
                        active = false;
                        ++cm.collected;
                        continue;
                    }
                    DrawTexturePro(atlas, {80, 22, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {coord.first, coord.second, 10, 10}, {0, 0}, 0.f, WHITE);
                }
            }

            for (auto& m: ms) {
                m.move(dt, p);
                if (CheckCollisionRecs({p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {m.pos.x-gc::R_PLAYER, m.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT})){
                    running = false;
                    win = false;
                }
                DrawTexturePro(atlas, m.rect, {m.pos.x-gc::R_PLAYER, m.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {2, 5}, 0.f, WHITE);
            }


            if (CheckCollisionRecs({p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT}, {monster_spawn_vec.x, monster_spawn_vec.y, 10,10}) && cm.collected == cm.coins.size()){
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