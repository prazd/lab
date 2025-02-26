#include "raylib.h"
#include <vector>
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

template <std::size_t sz>
struct HpBar {
    std::array<Vector2, sz> arr{};
    size_t count = sz;
    HpBar(float x, float y, float offset = 32.f){
        std::generate(arr.begin(), arr.end(), [x,y,offset]()mutable{return Vector2{std::exchange(x, x+offset), y};});
    }
    auto begin(){
        return arr.begin();
    }
    auto end() {
        return arr.begin() + count;
    }
};

template <typename Iter>
void draw_list_as_rec(Iter it, Iter ite, float width, float height, Color color = BLACK) {
    for (; it != ite; ++it) {
        DrawRectangleRec((Rectangle){it->x, it->y, width, height}, color);
    }
}

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

struct SuricaneAnimation {
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

struct Suricane {
    SuricaneAnimation anim{
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

    enum class State {none, left, right} state = State::none;

    void throw_suricane(State s){
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
struct SuricanePack{
    std::array<Suricane, n> suricanes;
    bool throw_suricane(Suricane::State s, float x, float y) {
        for (auto& el: suricanes){
            if (!el.in_process) {
                el.pos.x = x;
                el.pos.y = y;
                el.throw_suricane(s);
                return true;
            }
        }
        return false;
    }

    size_t ready() {
        size_t res{};
        for (auto& el: suricanes){
            if (el.in_process) {
                ++res;
            }
        }
        return n - res;
    }
};

template <size_t n = 1>
struct NinjaPlayer {
    SuricanePack<n> sp;
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

    void throw_suricane() {
        fcounter_throw = 1;
        static_animation.direction = movement_animation.direction;
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

template <size_t sz>
struct Enemies  {
    static inline std::array<NinjaPlayer<1>, sz> gang;
    size_t killed = 0;
    Enemies() {
        std::for_each(gang.begin(), gang.end(), [](auto& el){el.pos.x = !(rand() % 2) ? (el.movement_animation.direction = NinjaAnimation::Direction::right, -100 + (rand() % 10)) : (el.movement_animation.direction = NinjaAnimation::Direction::left, WIDTH + 100 + (rand() % 10));});
    }
    void raise(NinjaPlayer<1>& el) {
            el = NinjaPlayer{};
            el.pos.x = !(rand() % 2) ? (el.movement_animation.direction = NinjaAnimation::Direction::right, -100 + (rand() % 10)) : (el.movement_animation.direction = NinjaAnimation::Direction::left, WIDTH + 100 + (rand() % 10));
        
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
    Enemies<4> enemies;
    HpBar<5> hp_bar(10, 10);

    // Suricane suricane;
    std::stringstream ss_scores;
    std::stringstream ss_suricanes;
    ss_scores << "Scores:" << 0;
    ss_suricanes << "Suricanes:" << 0;
    auto stream_updater = [](auto& ss, auto&&... val){
        ss.str("");
        (ss << ... << val);
    };

    RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);
    auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(WHITE);
            auto dt = GetFrameTime();

            if (player.js == decltype(player)::JumpState::none)
                player.update_rect();

            if (IsKeyPressed(KEY_SPACE) && player.js == decltype(player)::JumpState::none)
                player.jump();

            if (IsKeyPressed(KEY_E)) {
                float x, y;
                if (player.movement_animation.direction == NinjaAnimation::Direction::left) {
                    x = player.pos.x + 60;
                    y = player.pos.y + 40;
                } else {
                    x = player.pos.x + 30;
                    y = player.pos.y + 40;
                }
                if (player.sp.throw_suricane(player.movement_animation.direction == NinjaAnimation::Direction::left ?
                    Suricane::State::left : Suricane::State::right, x, y)){
                        player.throw_suricane();
                }
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

            if (player.js != decltype(player)::JumpState::none)
                player.jump_proccess(dt);

            if (player.fcounter_throw)
                player.update_rect();


            DrawTexturePro(ninja, player.rect, {player.pos.x, player.pos.y, 100, 100}, {0, 0}, 0.0f, WHITE);
            DrawRectangleLinesEx({player.pos.x, player.pos.y, 100, 100}, 1, RED);

            for (auto& el: enemies.gang){
                el.update_rect();
                if (el.pos.x < WIDTH/2 && !el.killed) {
                    el.move_right(dt / 10);
                } else if (el.pos.x > 0 && !el.killed){
                    el.move_left(dt / 10);
                }

                if (el.js != NinjaPlayer<1>::JumpState::none)
                    el.jump_proccess(dt);
                
                DrawTexturePro(ninja, el.rect, {el.pos.x, el.pos.y, 100, 100}, {0, 0}, 0.0f, GREEN);
            }

            for (auto it = enemies.gang.begin(), ite = enemies.gang.end(); it != ite; ++it){
                for (auto& suricane: player.sp.suricanes){
                    if (suricane.state != Suricane::State::none && !it->killed && CheckCollisionRecs((Rectangle){suricane.pos.x, suricane.pos.y, 10, 10},
                                        (Rectangle){it->pos.x, it->pos.y, 100, 100})){
                        suricane.state = Suricane::State::none;
                        suricane.in_process = false;
                        it->dead_hit();
                    }

                    if (it->killed && !it->fcounter_killed && it->js == NinjaPlayer<1>::JumpState::none) {
                        enemies.raise(*it);
                        ++enemies.killed;
                        stream_updater(ss_scores, "Scores:", enemies.killed);
                        break;
                    }
                }
            }

            stream_updater(ss_suricanes, "Suricanes:", player.sp.ready());
            DrawText(ss_scores.str().data(), 400, 10, 20, BLACK);
            DrawText(ss_suricanes.str().data(), 600, 10, 20, BLACK);
            draw_list_as_textures(hp_bar.begin(), hp_bar.end(), heart);

            for (auto& suricane: player.sp.suricanes){
                if (suricane.state != Suricane::State::none) {
                    suricane.process(dt);
                    DrawTexturePro(ninja, suricane.rect,
                                   {suricane.pos.x, suricane.pos.y, 75, 75},
                                   {suricane.rect.width/2, suricane.rect.height/2},
                                   suricane.anim.angle, WHITE);
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