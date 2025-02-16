#include <iostream>
#include "raylib.h"

constexpr int HEIGHT = 600;
constexpr int WIDTH = 1200;
constexpr float NINJA_FRAME_WIDTH  = 88.f;
constexpr float NINJA_FRAME_HEIGHT = 80.f;
constexpr auto ninja_img_path = "/Users/prazd/Desktop/books/cpp/cpp/raylib/animation/img/ninja.png";
Texture2D ninja;

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
    };;
    Vector2 velocity{620,0};
    Vector2 pos{10,HEIGHT-200};
    enum class State {none, left, right} state = State::none;

    void throw_suricane(State s){
        state = s;
    }

    void process(float dt) {
        anim.update();
        switch (state) {
            case State::right:{
                if (pos.x < WIDTH){
                    pos.x += velocity.x * dt;
                } else {
                    state = State::none;
                }
                break;
            }
            case State::left: {
                if (pos.x > 0){
                    pos.x += -velocity.x * dt;
                } else {
                    state = State::none;
                }
                break;
            }
            default:
                break;
        }
    }
};

struct NinjaPlayer {
    NinjaAnimation movement_animation{
        .first = 0,
        .last = 3,
        .cur = 0,
        .speed = 0.1f,
        .duration_left = 0.1f
    };

    NinjaAnimation throw_animation{
        .first = 0,
        .last = 1,
        .cur = 0,
        .speed = 0.1f,
        .duration_left = 0.1f
    };

    Rectangle rect;
    Vector2 velocity{200,200};
    Vector2 pos{10,HEIGHT-200};
    enum class JumpState {none, up, down} js = JumpState::none;
    void jump() {js = JumpState::up;}

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

    void update_rect() {
        rect = movement_animation.animation_frame(10);
    }
};


void load_sprites() {
    auto img = LoadImage(ninja_img_path);
    ninja = LoadTextureFromImage(img);
}


int main(){
    InitWindow(WIDTH, HEIGHT, "Animation");
    load_sprites();

    SetTargetFPS(60);

    Rectangle floor{0, HEIGHT-100, WIDTH, 100};
    NinjaPlayer player;
    Suricane suricane;

    RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);
    auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(WHITE);

            auto dt = GetFrameTime();

            if (player.js == NinjaPlayer::JumpState::none)
                player.update_rect();

            if (IsKeyPressed(KEY_SPACE) && player.js == NinjaPlayer::JumpState::none)
                player.jump();


            if (IsKeyPressed(KEY_E) && suricane.state == Suricane::State::none) {
                if (player.movement_animation.direction == NinjaAnimation::Direction::left) {
                    suricane.pos.x = player.pos.x + 60;
                    suricane.pos.y = player.pos.y + 40;
                } else {
                    suricane.pos.x = player.pos.x + 30;
                    suricane.pos.y = player.pos.y + 40;
                }
                suricane.throw_suricane(
                        player.movement_animation.direction == NinjaAnimation::Direction::left ?
                Suricane::State::left : Suricane::State::right);
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

            if (player.js != NinjaPlayer::JumpState::none)
                player.jump_proccess(dt);

            DrawTexturePro(ninja, player.rect, {player.pos.x, player.pos.y, 100, 100}, {0, 0}, 0.0f, WHITE);
            if (suricane.state != Suricane::State::none) {
                suricane.process(dt);
                DrawTexturePro(ninja, suricane.rect,
                               {suricane.pos.x, suricane.pos.y, 75, 75},
                               {suricane.rect.width/2, suricane.rect.height/2},
                               suricane.anim.angle, WHITE);
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