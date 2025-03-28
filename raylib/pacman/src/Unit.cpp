#include "Unit.h"
#include "Constants.h"
#include <algorithm>
#include "Map.h"
#include "Random.h"
#include <iostream>

//#define DRAW_LINE = 1
//#define DEBUG_LOGS = 1

Unit::Unit(const Animation& anim, Vector2 pos, Vector2 velocity, Direction dir, Direction dir_next):
        anim(anim),  pos(pos), velocity(velocity), dir(dir), dir_next(dir_next){
}

void Unit::move_up(float dt) {
    pos.y -= velocity.y * dt;
}

void Unit::move_down(float dt) {
    pos.y += velocity.y * dt;
}

void Unit::move_left(float dt) {
    pos.x -= velocity.x * dt;
}

void Unit::move_right(float dt){
    pos.x += velocity.x * dt;
}

PacMan::PacMan():
Unit(Animation({.first = 0,
                       .last = 2,
                       .cur = 2,
                       .speed = 0.07f,
                       .duration = 0.1f,
                       .width = gc::FRAME_WIDTH,
                       .height = gc::FRAME_HEIGHT}),
     Coords::get_point('p'),
     {gc::PLAYER_VEL_X, gc::PLAYER_VEL_Y},
     Direction::null,
     Direction::null) {
        rect = anim.animation_frame_adaptive(10, 0, 22),
        pos.x += gc::PLAYER_SPAWN_OFFSET_X;
        pos.y += gc::PLAYER_SPAWN_OFFSET_Y;
        anim.set_direction(dir);
}

bool Unit::collision(Direction d) const {
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
        DrawRectangleLinesEx((Rectangle){current_x, current_y, gc::QUBE_SIDE, gc::QUBE_SIDE}, 1, GREEN);
#endif
        return std::any_of(std::begin(Walls::points), std::end(Walls::points), [current_x, current_y](auto& el){
            if (CheckCollisionRecs((Rectangle){current_x, current_y, gc::QUBE_SIDE, gc::QUBE_SIDE},
                                   (Rectangle){el.x, el.y, gc::OFFSET, gc::OFFSET})){
#ifdef DRAW_LINE
                DrawRectangleLinesEx((Rectangle){el.x, el.y, gc::QUBE_SIDE, gc::QUBE_SIDE}, 1, YELLOW);
#endif
            }
            return CheckCollisionRecs((Rectangle){current_x, current_y, gc::QUBE_SIDE, gc::QUBE_SIDE},
                                      (Rectangle){el.x, el.y, gc::OFFSET, gc::OFFSET});
        });
}

void PacMan::update(Direction d) {
    dir_next = d;
}

void PacMan::move(float dt) {
    rect = anim.animation_frame_adaptive(10, 0, 22);
    if (dir_next != Direction::null && !collision(dir_next)) {
        dir = dir_next;
        anim.set_direction(dir);
        dir_next = Direction::null;
    }
    if (collision(dir)){
        dir = Direction::null;
        anim.d.cur = anim.d.last;
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

Ghost::Ghost():
        Unit(Animation({
                .first = 0,
                .last = 1,
                .cur = 0,
                .speed = 0.07f,
                .duration = 0.1f,
                .width = gc::MONSTER_FRAME_WIDTH,
                .height = gc::MONSTER_FRAME_HEIGHT
              }),
             {},
             {gc::PLAYER_VEL_X, gc::PLAYER_VEL_Y},
             Direction::right,
             Direction::null), offset_x(3), offset_y(0) {

                Map::call([this](auto, auto coord_i, auto el) {
                    if(el == '@') {
                        auto [x, y] = Coords::points[coord_i];
                        pos = {x + 21, y + 21};
                    }
                });

                switch (Random<int, 1,4>::roll()) {
                    case 1: {offset_y = gc::BLINKY_OFFSET_Y; break;}
                    case 2: {offset_y = gc::PINKY_OFFSET_Y;  break;}
                    case 3: {offset_y = gc::INKY_OFFSET_Y;   break;}
                    case 4: {offset_y = gc::CLYDE_OFFSET_Y;  break;}
                }

                rect = anim.animation_frame(10, offset_x, offset_y);
                anim.set_direction(dir);
}

void Ghost::move(float dt, const Unit& p) {
    // left, right, up, down
    std::array<bool, 4> current{collision(Direction::left), collision(Direction::right), collision(Direction::up), collision(Direction::down)};

    if (dir != Direction::null) {
#ifdef  DEBUG_LOGS
        if (CheckCollisionRecs(ray, {p.pos.x-gc::R_PLAYER, p.pos.y-gc::R_PLAYER, gc::FRAME_WIDTH, gc::FRAME_HEIGHT})){
                        std::cout << "----------------------" << '\n';
                        std::cout << "PX:"    <<    p.pos.x-gc::R_PLAYER << '\n';
                        std::cout << "PY:"    <<    p.pos.y-gc::R_PLAYER << '\n';
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
        case Direction::left:  { move_left(dt);  anim.update();  rect = anim.animation_frame(10, offset_x, offset_y); break;}
        case Direction::right: { move_right(dt); anim.update();  rect = anim.animation_frame(10, offset_x+70, offset_y); break;}
        case Direction::up:    { move_up(dt);    anim.update();  rect = anim.animation_frame(10, offset_x+140, offset_y); break;}
        case Direction::down:  { move_down(dt);  anim.update();  rect = anim.animation_frame(10, offset_x+210, offset_y); break;}
        default: break;
    }
    update_ray();
}


void Ghost::update_ray() {
    switch(dir) {
        case Direction::left:   {ray = {pos.x-gc::MONSTER_VIEW_DISTANCE, pos.y, gc::MONSTER_VIEW_DISTANCE, 1};break;}
        case Direction::right:  {ray = {pos.x, pos.y, gc::MONSTER_VIEW_DISTANCE, 1};break;}
        case Direction::up:     {ray = {pos.x, pos.y-gc::MONSTER_VIEW_DISTANCE, 1, gc::MONSTER_VIEW_DISTANCE};break;}
        case Direction::down:   {ray = {pos.x, pos.y, 1, gc::MONSTER_VIEW_DISTANCE};break;}
        default: break;
    }
}