#include "Animation.h"
#include "raylib.h"

Animation::Animation(const AnimationData &d): d(d){}

void Animation::update() {
    float dt = GetFrameTime();
        d.duration-=dt;
    if (d.duration <= 0.f){
        d.duration = d.speed;
        d.cur = d.cur == d.last ? d.first : ++d.cur;
    }
}

Rectangle Animation::animation_frame_adaptive(int num_frames_per_row, float offset_x, float offset_y) {
    float x = float(d.cur % num_frames_per_row) * d.width;
    float y = float(d.cur / num_frames_per_row) * d.height;
    return (Rectangle){
            .x = x + offset_x,
            .y = y + offset_y,
            .width =  ((d.direction == Direction::left || d.direction == Direction::up) ? -d.width : d.width),
            .height = d.height
    };
}

Rectangle Animation::animation_frame(int num_frames_per_row, float offset_x, float offset_y) {
    float x = float(d.cur % num_frames_per_row) * d.width;
    float y = float(d.cur / num_frames_per_row) * d.height;
    return (Rectangle){
            .x = x + offset_x,
            .y = y + offset_y,
            .width =  d.width,
            .height = d.height
    };
}

Animation& Animation::operator=(const AnimationData& ad) {
    d = ad;
    return *this;
}

void Animation::set_direction(Direction dir) {
    d.direction = dir;
}