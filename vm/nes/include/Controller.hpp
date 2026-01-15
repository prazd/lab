#ifndef NES_CONTROLLER_H
#define NES_CONTROLLER_H
#include "raylib.h"
struct Controller {
    enum Button {
        A, B,
        Select, Start,
        Up, Down,
        Left, Right
    };

    unsigned char status1{};
    unsigned char current1 = 0;
    bool latch1 = false;

    KeyboardKey keys1[8] = {
        KEY_Q,KEY_D,
        KEY_SPACE,KEY_ENTER,
        KEY_UP,KEY_DOWN,
        KEY_LEFT, KEY_RIGHT
    };

    inline void update_status1() {
        if (latch1) {
            status1 = 0;
            for (int i = 0; i < 8; ++i)
                status1 |= (IsKeyDown(keys1[i]) << i);
            current1 = 0;
            latch1 = false;
        }
    }

    inline bool read_status1(){
        if (current1++ < 8) {
            auto res = status1 & 1;
            status1 >>= 1;
            return res;
        }
        return true;
    }
};
#endif //NES_CONTROLLER_H
