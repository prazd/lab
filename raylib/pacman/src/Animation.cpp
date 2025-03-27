#include "Animation.h"
#include "raylib.h"

void Animation::update() {
    float dt = GetFrameTime();
        duration-=dt;
    if (duration <= 0.f){
        duration = speed;
        cur = cur == last ? first : ++cur;
    }
}

Rectangle Animation::animation_frame_adaptive(int num_frames_per_row, float offset_x, float offset_y) {
    float x = float(cur % num_frames_per_row) * width;
    float y = float(cur / num_frames_per_row) * height;
    return (Rectangle){
            .x = x + offset_x,
            .y = y + offset_y,
            .width =  ((direction == Direction::left || direction == Direction::up) ? -width : width),
            .height = height
    };
}

Rectangle Animation::animation_frame(int num_frames_per_row, float offset_x, float offset_y) {
    float x = float(cur % num_frames_per_row) * width;
    float y = float(cur / num_frames_per_row) * height;
    return (Rectangle){
            .x = x + offset_x,
            .y = y + offset_y,
            .width =  width,
            .height = height
    };
}

void Animation::set_direction(Direction dir) {
    direction = dir;
}