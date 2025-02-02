#include <iostream>
#include <random>
#include "raylib.h"
#include <array>

constexpr int HEIGHT = 600;
constexpr int WIDTH = 1200;
constexpr int BLOCK_H = 40;
constexpr int BLOCK_W = 60;
constexpr int BALL_SPEED = 7;
constexpr int START_POS_X = 550;
constexpr int START_POS_Y = 500;
constexpr int INVADERS_COUNT = 10;
constexpr auto ship_img_path = "/Users/prazd/Desktop/books/cpp/cpp/raylib/invaders/img/ship.png";
constexpr auto invader_img_path = "/Users/prazd/Desktop/books/cpp/cpp/raylib/invaders/img/invader.png";
constexpr auto background_img_path = "/Users/prazd/Desktop/books/cpp/cpp/raylib/invaders/img/background.jpg";
Texture2D ship_sprite;
Texture2D invader_sprite;
Texture2D background_sprite;

template <int begin, int end>
class Random {
    static inline std::random_device rd;
    static inline std::mt19937 gen{rd()};
    static inline std::uniform_int_distribution<int> dis = std::uniform_int_distribution<int>(begin, end);
public:
    static int roll() {
        return dis(gen);
    }
};

void load_sprites() {
    // init ship sprite
    auto img = LoadImage(ship_img_path);
    ImageResize(&img, 80, 60);
    ship_sprite = LoadTextureFromImage(img);

    // init invader sprite
    img = LoadImage(invader_img_path);
    ImageResize(&img, 80, 60);
    invader_sprite = LoadTextureFromImage(img);

    // background
    img = LoadImage(background_img_path);
    ImageResize(&img, WIDTH, HEIGHT);
    background_sprite = LoadTextureFromImage(img);
}

struct HP{
    Rectangle rec;
    Color color;
    bool active;
};

template <size_t sz>
struct HPBar {
    std::array<HP, sz> bar;
    size_t active_index = sz-1;
    HPBar() {
        float x = 60;
        float y = 580;
        float step = 60;
        for (auto& el: bar) {
            el = {{x, y, 40, 10}, GREEN, true};
            x += step;
        }
    }
    auto begin() {
        return bar.begin();
    }
    auto end() {
        return bar.end();
    }
    void reset() {
        for(auto& el: bar)
            el.active = true;
        active_index = sz-1;
    }
    void min_hp() {
        if (active_index) bar[active_index--].active = false;
    }
};

struct Spaceship {
    Rectangle rec;
    void move() {
        auto mouse_pos = GetMousePosition();
        rec.x = (mouse_pos.x-rec.width/2);
    }
};

struct Invader {
    Rectangle rec;
    bool active;
    static void left_move(Invader& inv, float x) {
        inv.rec.x -= x;
    }
    static void right_move(Invader& inv, float x) {
        inv.rec.x += x;
    }
};

template <size_t n>
struct Invaders {
    std::array<Invader, n> gang;
    size_t active;
    enum class MovinState: char {Left, Right} current = MovinState::Left;
    Invaders() {
        float start_x = 25;
        float start_y = 20;
        for(size_t i = 0; i < n; ++i) {
            gang[i] = {{start_x, start_y, 80, 60}, true};
            start_x += 80;
            ++active;
        }        
    }

    size_t get_active() const {
        return active;
    }

    void move() {
        void (*current_move)(Invader&, float) = nullptr;
        if (current == MovinState::Left) {
            current_move = &Invader::left_move;
        } else {
            current_move = &Invader::right_move;
        }
        for (auto& el: gang)
            current_move(el, 1.f);
        
        const auto& right_inv = gang[n-1];
        const auto& left_inv = gang[0];
        if ((right_inv.rec.x + right_inv.rec.width >= WIDTH && current == MovinState::Right) || (left_inv.rec.x <= 0 && current == MovinState::Left)) {
            current = current == MovinState::Left ? MovinState::Right : MovinState::Left;
        }
    }
};

struct Bullet {
    Rectangle rec;
    Vector2 velocity;
    Color color;
    bool active;

    void move(float vely, float velx = 0.f) {
        rec.y += velocity.y;
        rec.x += velocity.x;
        velocity.y += vely;
        velocity.x += velx;
    }

    void activate(int x, int y) {
        rec.x = x;
        rec.y = y;
        active = true;
    }
};

template <typename Invaders>
struct ObjectsManager {
    Spaceship& sp;
    Bullet& shipb;
    Bullet& invb;
    Invaders& invs;
    bool invb_left = false;
    ObjectsManager(Spaceship& sp, Bullet& shipb, Bullet& invb, Invaders& invs): sp(sp), shipb(shipb), invb(invb), invs(invs) {}
    
    void ship_move() {
        auto mouse_pos = GetMousePosition();
        sp.rec.x = (mouse_pos.x-sp.rec.width/2);
    }
    
    bool collision(int lx, int ly, int lw, int lh, int rx, int ry, int rw, int rh) {}
    void check_bullet_collision(Bullet& b, auto predfunc) {
        if (predfunc(b)) {
            b.active = false;
            b.velocity.y = 0.f;
            b.velocity.x = 0.f;
        }
    }
    void update() {
        ship_move();
        if (!invb.active) {
            // optimize
            std::array<Invader, INVADERS_COUNT> active;
            std::copy_if(invs.gang.begin(), invs.gang.end(), active.begin(), [](auto& el){return el.active;});
            auto id = Random<0, INVADERS_COUNT>::roll() % active.size();
            invb.activate(active[id].rec.x + active[id].rec.width/2, active[id].rec.y);
            invb_left = (invb.rec.x > sp.rec.x);
        }
        if (invb.active) {
            invb.move(0.2f, invb_left ? -0.2f : 0.2f);
            check_bullet_collision(invb, [](auto& b){return b.rec.y >= HEIGHT;});
        }
        
        if ((IsKeyPressed(KEY_SPACE)) && !shipb.active) {
            shipb.activate(sp.rec.x + sp.rec.width/2 - 26, sp.rec.y);
        }

        if (shipb.active){
            shipb.move(-0.2f);
            check_bullet_collision(shipb, [](auto& b){return b.rec.y <= 0;});
        };
        invs.move();
    }
};

int main(){
    InitWindow(WIDTH, HEIGHT, "Ivaders");
    load_sprites();
    Spaceship sp{{START_POS_X, START_POS_Y, 120,40}};
    Bullet shipb{{START_POS_X, START_POS_Y, 10,10},{0.f, 0.f}, RED, false};
    Bullet invb{{START_POS_X, START_POS_Y, 10,10},{0.f, 0.f}, GREEN, false};
    Invaders<INVADERS_COUNT> invs;

    HPBar<5> bar;

    ObjectsManager om(sp, shipb, invb, invs);
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);
    auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(WHITE);
            DrawTexture(background_sprite, 0, 0, WHITE);
            DrawTexture(ship_sprite, sp.rec.x, sp.rec.y, WHITE);
            if (om.shipb.active) DrawRectangleRec(shipb.rec, shipb.color);  // ship bullet
            if (om.invb.active) DrawRectangleRec(invb.rec, invb.color);     // invaders bullet

            for(auto& el: invs.gang) 
                if (el.active) DrawTexture(invader_sprite, el.rec.x, el.rec.y, WHITE);
            
            for(const auto& el: bar)
                if (el.active) DrawRectangleRec(el.rec, el.color);   // hp bar
            
        EndTextureMode();
    };
   
  while(!WindowShouldClose()){
        om.update();
        render();
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height}, {0, 0}, WHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }

    UnloadTexture(ship_sprite);
    UnloadTexture(invader_sprite);
    UnloadTexture(background_sprite);
    CloseWindow();
}
