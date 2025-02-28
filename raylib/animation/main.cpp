#include "raylib.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <utility>

constexpr int HEIGHT = 600;
constexpr int WIDTH = 1200;
constexpr float PLAYER_START_POS_X = WIDTH/2;
constexpr float PLAYER_START_POS_Y = HEIGHT-200;
constexpr float NINJA_FRAME_WIDTH  = 88.f;
constexpr float NINJA_FRAME_HEIGHT = 80.f;
constexpr auto ninja_img_path = "/Users/prazd/Desktop/books/cpp/cpp/raylib/animation/img/ninja.png";
constexpr auto heart_image_path = "/Users/prazd/Desktop/books/cpp/cpp/raylib/animation/img/pixel_heart.png";
Texture2D ninja;
Texture2D heart;
constexpr Vector2 heart_sz = {30,30};

template <int sz>
struct HpBar {
    std::array<Vector2, sz> arr{};
    int count = sz;
    HpBar(float x, float y, float offset = 32.f){
        std::generate(arr.begin(), arr.end(), [x,y,offset]()mutable{return Vector2{std::exchange(x, x+offset), y};});
    }
    void add_hp() {
        if (count < sz) {
            ++count;
        }
    }
    void min_hp() {
        --count;
    }
    bool is_alive() const {
        return count > 0;
    }
    auto begin(){
        return arr.begin();
    }
    auto end() {
        return arr.begin() + count;
    }
};

template <typename Iter>
void draw_list_as_textures(Iter it, Iter ite, Texture2D& texture) {
    for (; it != ite; ++it) {
        DrawTexture(texture, it->x, it->y, WHITE);
    }
}

struct NinjaAnimation {
    enum class Direction {
        left = -1, right = 1
    } direction = Direction::right;
    int first;
    int last;
    int cur;
    float speed;
    float duration_left;
    void update() {
        float dt = GetFrameTime();
        duration_left-=dt;
        if (duration_left <= 0.f) {
            duration_left = speed;
            cur = cur > last ? first : ++cur;
        }
    }
    Rectangle animation_frame(int num_frames_per_row, float offset_x = 0.f, float offset_y = 0.f) {
        float x = float(cur % num_frames_per_row) * NINJA_FRAME_WIDTH;
        float y = float(cur / num_frames_per_row) * NINJA_FRAME_HEIGHT;
        return (Rectangle){
            .x = x + offset_x,
            .y = y + offset_y,
            .width = (direction == Direction::left ? -NINJA_FRAME_WIDTH : NINJA_FRAME_WIDTH),
            .height = NINJA_FRAME_HEIGHT
        };
    }

    void set_direction(Direction dir) {
        direction = dir;
    }
};

struct ShurikenAnimation {
    float speed;
    float duration_left;
    float angle = 0.f;
    void update() {
        float dt = GetFrameTime();
        duration_left-=dt;
        if (duration_left <= 0.f) {
            duration_left = speed;
            angle += 30.f;
            if (angle == 360.f) angle = 0.f;
        }
    }
};

struct Shuriken {
    ShurikenAnimation anim{
        .speed = 0.001f,
        .duration_left = 0.1f
    };
    Rectangle rect{
        .x = 0,
        .y = 352,
        .width = NINJA_FRAME_WIDTH,
        .height = NINJA_FRAME_HEIGHT
    };
    Vector2 velocity{620,0};
    Vector2 pos{10,HEIGHT-200};
    bool in_process = false;

    void stop() {
        in_process = false;
        state = State::none;
    }

    enum class State {none, left, right} state = State::none;

    void throw_shuriken(State s){
        in_process = (s == State::left || s == State::right);
        state = s;
    }

    void process(float dt) {
        anim.update();
        switch (state) {
            case State::right: {
                if (pos.x < WIDTH){
                    pos.x += velocity.x * dt;
                } else {
                    state = State::none;
                    in_process = false;
                }
                break;
            }
            case State::left: {
                if (pos.x > 0){
                    pos.x += -velocity.x * dt;
                } else {
                    state = State::none;
                    in_process = false;
                }
                break;
            }
            default:
                break;
        }
    }
};

template <size_t n>
struct ShurikenPack{
    std::array<Shuriken, n> shurikens;
    bool throw_shuriken(Shuriken::State s, float x, float y) {
        for (auto& el: shurikens){
            if (!el.in_process) {
                el.pos.x = x;
                el.pos.y = y;
                el.throw_shuriken(s);
                return true;
            }
        }
        return false;
    }

    size_t ready() {
        size_t res{};
        for (auto& el: shurikens){
            if (el.in_process) {
                ++res;
            }
        }
        return n - res;
    }
};

template <size_t n = 1>
struct NinjaPlayer {
    ShurikenPack<n> sp;
    NinjaAnimation movement_animation{
        .first = 0,
        .last = 3,
        .cur = 0,
        .speed = 0.1f,
        .duration_left = 0.1f
    };

    NinjaAnimation static_animation{
        .first = 0,
        .last = 1,
        .cur = 0,
        .speed = 0.5f,
        .duration_left = 0.5f
    };

    bool killed = false;
    int fcounter_throw = 0;
    int fcounter_killed = 0;

    Rectangle rect;
    Vector2 velocity{200,200};
    Vector2 pos{PLAYER_START_POS_X,PLAYER_START_POS_Y};

    enum class JumpState {none, up, down} js = JumpState::none;

    void jump() {js = JumpState::up;}

    void throw_shuriken() {
        float x, y;
        if (movement_animation.direction == NinjaAnimation::Direction::left) {
            x = pos.x + 60;
            y = pos.y + 40;
        } else {
            x = pos.x + 30;
            y = pos.y + 40;
        }
        if (sp.throw_shuriken(movement_animation.direction == NinjaAnimation::Direction::left ?
            Shuriken::State::left : Shuriken::State::right, x, y)){
                fcounter_throw = 1;
                static_animation.direction = movement_animation.direction;
        }
    }

    void dead_hit() {
        fcounter_killed = 1;
        killed = true;
        js = JumpState::up;
        static_animation.direction = movement_animation.direction;
    }


    void jump_proccess(float dt) {
        switch (js){
            case JumpState::up: {
                if (pos.y >= HEIGHT-300){
                    pos.y += -velocity.y * dt;
                } else {
                    js = JumpState::down;
                }
                break;
            }
            case JumpState::down:{
                if (pos.y <= HEIGHT-200){
                    pos.y += velocity.y * dt;
                } else {
                    js = JumpState::none;
                }
                break;
            }
            default: break;
        }
    }

    void set_dir_left(){
        movement_animation.set_direction(NinjaAnimation::Direction::left);
    }

    void set_dir_right() {
        movement_animation.set_direction(NinjaAnimation::Direction::right);
    }

    void move_right(float dt) {
        pos.x += velocity.x * dt;
        movement_animation.update();
    }

    void move_left(float dt) {
        pos.x += -velocity.x * dt;
        movement_animation.update();
    }

    void stop_move() {
        movement_animation.cur = movement_animation.first;
        movement_animation.update();
    }

    template <int more, int less, int offset_x, int offset_y>
    static void processed_static_animation(Rectangle& rect, NinjaAnimation& animation, int& counter) {
        if (counter > more && counter < less) {
            rect = animation.animation_frame(10, offset_x, offset_y);
            ++counter;
            return;
        }
        counter = 0;
    }

    void update_rect() {
        if (fcounter_throw > 0){
            processed_static_animation<0, 22, 88, 88>(rect, static_animation, fcounter_throw);
            return;
        }

        if (fcounter_killed > 0) {
            processed_static_animation<0, 80, 88, 264>(rect, static_animation, fcounter_killed);
            return;
        }

        rect = movement_animation.animation_frame(10);

    }
};

template <size_t sz, size_t shuriken_count = 1>
struct NinjaEnemies  {
    static inline std::array<NinjaPlayer<shuriken_count>, sz> gang;
    size_t killed = 0;
    NinjaEnemies() {
        std::for_each(gang.begin(), gang.end(), [](auto& el){el.pos.x = !(rand() % 2) ? (el.movement_animation.direction = NinjaAnimation::Direction::right, -100 + (rand() % 10)) : (el.movement_animation.direction = NinjaAnimation::Direction::left, WIDTH + 100 + (rand() % 10));});
    }
    void raise(auto& el) {
            el = NinjaPlayer{};
            el.pos.x = !(rand() % 2) ? (el.movement_animation.direction = NinjaAnimation::Direction::right, -100 + (rand() % 10)) : (el.movement_animation.direction = NinjaAnimation::Direction::left, WIDTH + 100 + (rand() % 10));
    }
    void flush() {
        killed = 0;
        std::for_each(gang.begin(), gang.end(), [](auto& el){el.pos.x = !(rand() % 2) ? (el.movement_animation.direction = NinjaAnimation::Direction::right, -100 + (rand() % 10)) : (el.movement_animation.direction = NinjaAnimation::Direction::left, WIDTH + 100 + (rand() % 10));});
        std::for_each(gang.begin(), gang.end(), [](auto& el){
            for (auto& Shuriken: el.sp.shurikens){
                Shuriken.state = Shuriken::State::none;
            }
        });
    }
};


void load_sprites() {
    ninja = LoadTextureFromImage(LoadImage(ninja_img_path));
    auto img_heart = LoadImage(heart_image_path);
    ImageResize(&img_heart, heart_sz.x, heart_sz.y);
    heart = LoadTextureFromImage(img_heart);
}

int main(){
    InitWindow(WIDTH, HEIGHT, "Animation");
    load_sprites();
    SetTargetFPS(60);

    Rectangle floor{0, HEIGHT-100, WIDTH, 100};
    NinjaPlayer<3> player;
    NinjaEnemies<5> enemies;
    HpBar<5> hp_bar(10, 10);

    using Player = decltype(player);
    using Enemies = decltype(enemies);
    using EnemyPlayer = decltype(Enemies::gang)::value_type;

    bool game_running = true;

    std::stringstream ss_scores;
    std::stringstream ss_Shurikens;
    ss_scores << "Scores:" << 0;
    ss_Shurikens << "shurikens:" << 0;

    auto stream_updater = [](auto& ss, auto&&... val){
        ss.str("");
        (ss << ... << val);
    };

    auto restart = [&]{
        game_running = true;
        player = Player{};
        enemies.flush();
        hp_bar = HpBar<5>(10,10);
        stream_updater(ss_scores, "Scores:", 0);
        stream_updater(ss_Shurikens, "Shurikens:", 0);
    };

    RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);
    auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(WHITE);
            if (!game_running) {
                DrawText("Game over. Press R to restart", (WIDTH/2)-200, HEIGHT/2, 20, BLACK);
                if (IsKeyPressed(KEY_R)) {
                    restart();
                }
                EndTextureMode();
                return;
            }

            auto dt = GetFrameTime();

            if (player.js == Player::JumpState::none)
                player.update_rect();

            if (IsKeyPressed(KEY_SPACE) && player.js == Player::JumpState::none)
                player.jump();

            if (IsKeyPressed(KEY_E)) {
                player.throw_shuriken();
            }

            if (IsKeyPressed(KEY_R)) {
                restart();
            }

            if (IsKeyPressed(KEY_A)){
                player.set_dir_left();
                player.update_rect();
            }

            if (IsKeyPressed(KEY_D)){
                player.set_dir_right();
                player.update_rect();
            }

            if (IsKeyDown(KEY_D))
                player.move_right(dt);
            
            if (IsKeyDown(KEY_A))
                player.move_left(dt);

            if (IsKeyReleased(KEY_A) || IsKeyReleased(KEY_D))
                player.stop_move();

            if (player.js != Player::JumpState::none)
                player.jump_proccess(dt);

            if (player.fcounter_throw)
                player.update_rect();


            // player <-> enemy shuriken collisison
            for (auto& player_shuriken: player.sp.shurikens) {
                for (auto& enemy: Enemies::gang) {
                    for (auto& enemy_shuriken: enemy.sp.shurikens) {
                        if (player_shuriken.state != Shuriken::State::none && enemy_shuriken.state != Shuriken::State::none && enemy_shuriken.in_process
                        && player_shuriken.in_process && CheckCollisionRecs((Rectangle){player_shuriken.pos.x, player_shuriken.pos.y, 10, 10},
                                                    (Rectangle){enemy_shuriken.pos.x, enemy_shuriken.pos.y, 10, 10})) {
                            player_shuriken.stop();
                            enemy_shuriken.stop();
                        }
                    }
                }
            }


            for (auto& enemy: Enemies::gang){
                // Player -> NinjaEnemies
                for (auto& shuriken: player.sp.shurikens){
                    if (shuriken.state != Shuriken::State::none && !enemy.killed && CheckCollisionRecs((Rectangle){shuriken.pos.x, shuriken.pos.y, 10, 10},
                                                                                                     (Rectangle){enemy.pos.x, enemy.pos.y, 100, 100})){
                        shuriken.stop();
                        enemy.dead_hit();
                    }

                }

                if (enemy.killed && !enemy.fcounter_killed && enemy.js == EnemyPlayer::JumpState::none) {
                    enemies.raise(enemy);
                    ++enemies.killed;
                    hp_bar.add_hp();
                    stream_updater(ss_scores, "Scores:", enemies.killed);
                    break;
                }

                // NinjaEnemies -> player
                if ((enemy.movement_animation.direction == NinjaAnimation::Direction::right && enemy.pos.x >= (WIDTH/2 - 500)) ||
                (enemy.movement_animation.direction == NinjaAnimation::Direction::left &&
                        enemy.pos.x <= WIDTH/2 + 400) && !enemy.killed){
                    enemy.throw_shuriken();
                }


                for (auto& shuriken: enemy.sp.shurikens){
                    if (shuriken.state != Shuriken::State::none && !enemy.killed && CheckCollisionRecs((Rectangle){shuriken.pos.x, shuriken.pos.y, 10, 10},
                                                                                                     (Rectangle){player.pos.x, player.pos.y, 100, 100})){
                        shuriken.stop();
                        hp_bar.min_hp();
                        if (!hp_bar.is_alive()) {
                            game_running = false;
                        }
                    }
                }
            }

            

            // Draw player
            DrawTexturePro(ninja, player.rect, {player.pos.x, player.pos.y, 100, 100}, {0, 0}, 0.0f, WHITE);
            DrawRectangleLinesEx({player.pos.x, player.pos.y, 100, 100}, 1, RED);

            // Draw enemies
            for (auto& el: Enemies::gang){
                el.update_rect();
                if (el.pos.x < (WIDTH/2 - 500) && !el.killed) {
                    el.move_right(dt / 5);
                } else if (el.pos.x > (WIDTH/2 + 400) && !el.killed){
                    el.move_left(dt / 5);
                }

                if (el.js != EnemyPlayer::JumpState::none)
                    el.jump_proccess(dt);

                DrawTexturePro(ninja, el.rect, {el.pos.x, el.pos.y, 100, 100}, {0, 0}, 0.0f, GREEN);
            }

            // Draw text info
            stream_updater(ss_Shurikens, "Shurikens:", player.sp.ready());
            DrawText(ss_scores.str().data(), 400, 10, 20, BLACK);
            DrawText(ss_Shurikens.str().data(), 600, 10, 20, BLACK);

            // Draw hp
            if (hp_bar.is_alive())
                draw_list_as_textures(hp_bar.begin(), hp_bar.end(), heart);

            // Draw player's shurikens
            for (auto& shuriken: player.sp.shurikens){
                if (shuriken.state != Shuriken::State::none) {
                    shuriken.process(dt);
                    DrawTexturePro(ninja, shuriken.rect,
                                   {shuriken.pos.x, shuriken.pos.y, 75, 75},
                                   {shuriken.rect.width / 2, shuriken.rect.height / 2},
                                   shuriken.anim.angle, WHITE);
                }
            }


            // Draw enemies shurikens
            for (auto& gang: Enemies::gang){
                for (auto& shuriken: gang.sp.shurikens) {
                    if (shuriken.state != Shuriken::State::none) {
                        shuriken.process(dt);
                        DrawTexturePro(ninja, shuriken.rect,
                                       {shuriken.pos.x, shuriken.pos.y, 75, 75},
                                       {shuriken.rect.width / 2, shuriken.rect.height / 2},
                                       shuriken.anim.angle, WHITE);
                    }
                }
            }


        DrawRectangleRec(floor, BLACK);
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

    UnloadTexture(ninja);
    CloseWindow();
}