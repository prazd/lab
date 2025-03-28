#ifndef PACMAN_UNIT_H
#define PACMAN_UNIT_H
#include "Animation.h"
#include "raylib.h"
class Unit {
public:
    Animation anim;
    Rectangle rect;
    Vector2 pos;
    Vector2 velocity;
    Direction dir;
    Direction dir_next;
    Unit(const Animation&, Vector2, Vector2, Direction, Direction);
    void move_up(float dt);
    void move_down(float dt);
    void move_left(float dt);
    void move_right(float dt);
    bool collision(Direction) const;
};

class PacMan: public Unit{
public:
    PacMan();
    void update(Direction d);
    void move(float dt);
};

class Ghost: public Unit {
    Rectangle ray;
    float offset_x;
    float offset_y;
public:
    Ghost();
    void move(float dt, const Unit& u);
    void update_ray();
};

#endif //PACMAN_UNIT_H
