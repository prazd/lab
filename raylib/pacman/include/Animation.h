#ifndef PACMAN_ANIMATION_H
#define PACMAN_ANIMATION_H
#include "raylib.h"

enum class Direction{left, right, up, down, null};

struct AnimationData{
    Direction direction;
    int first;
    int last;
    int cur;
    float speed;
    float duration;
    float width;
    float height;
};

class Animation {
public:
    AnimationData d;
    explicit Animation(const AnimationData&);
    Animation& operator=(const AnimationData&);
    void update();
    Rectangle animation_frame_adaptive(int num_frames_per_row, float offset_x = 0.f, float offset_y = 0.f);
    Rectangle animation_frame(int num_frames_per_row, float offset_x = 0.f, float offset_y = 0.f);
    void set_direction(Direction dir);
};

#endif //PACMAN_ANIMATION_H
