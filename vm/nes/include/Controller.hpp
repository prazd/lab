#ifndef NES_CONTROLLER_H
#define NES_CONTROLLER_H
#include "raylib.h"
#include "Constants.hpp"
struct Controller {
    enum Button {
        A, B,
        Select, Start,
        Up, Down,
        Left, Right
    };

    unsigned char status1{};
    unsigned char current1 = 0;
    bool prev_res = false;
    bool latch1 = false;
    bool shift_req = false;

    KeyboardKey keys1[8] = {
        KEY_Q,KEY_E,
        KEY_SPACE,KEY_ENTER,
        KEY_UP,KEY_DOWN,
        KEY_LEFT, KEY_RIGHT
    };
    KeyboardKey keys2[8] = {
            KEY_LEFT_BRACKET,KEY_RIGHT_BRACKET,
            KEY_SPACE,KEY_ENTER,
            KEY_W,KEY_S,
            KEY_A, KEY_D
    };
#if DEBUG_MODE
    unsigned char custom_status{};
    static void cs_switch(Controller& c, int i, bool dir){
        i += (dir << 2);
        c.custom_status ^= (1 << i);
    }
    static void cs_set(Controller& c, int i, bool dir, bool value){
        i += (dir << 2);
        c.custom_status &= ((~(1 << i)) & 0xff);
        c.custom_status |= (value << i);
    }
    bool cs_read(Controller& c, int i, bool dir) {
        i += (dir << 2);
        return (c.custom_status >> i) & 1;
    }
#endif

    inline void update_status1() {
        if (latch1) {
            status1 = 0;
            for (int i = 0; i < 8; ++i)
                status1 |=
#if DEBUG_MODE
                        ((custom_status & (1 << i)) | (IsKeyDown(keys1[i]) || IsKeyDown(keys2[i])) << i);
#else

                        ((IsKeyDown(keys1[i]) || IsKeyDown(keys2[i])) << i);
#endif
            current1 = 0;
        }
    }

    inline bool read_status1(){
        if (latch1) return IsKeyDown(keys1[0]) || IsKeyDown(keys2[0]);
        if (current1 < 8){
            prev_res = status1 & 1;
            return prev_res;
        }
        return true;
    }
};
#endif //NES_CONTROLLER_H
