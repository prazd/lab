#ifndef RAM_H_JOYPAD_H
#define RAM_H_JOYPAD_H
#include "RAM.h"
#include "raylib.h"
struct Joypad {
    enum JoypadKeys {
        A_Right, B_Left, Select_Up, Start_Down,
        NotPressed
    } button_state = NotPressed;

    enum Keys {
        Right, Left, Up,Down,
        A, B, Select, Start
    };

    KeyboardKey keyboard_keys[8]{
            KEY_D,     // RIGHT
            KEY_A,     // LEFT
            KEY_W,     // UP
            KEY_S,     // DOWN

            KEY_Q,     // B
            KEY_E,     // A
            KEY_ENTER, // Start
            KEY_SPACE, // Select
    };
    JoypadKeys key_shifts[8]{
            A_Right,
            B_Left,
            Select_Up,
            Start_Down,

            B_Left,
            A_Right,
            Start_Down,
            Select_Up,
    };


    bool is_select_buttons = false;
    void update_state(RAM&);
};
#endif //RAM_H_JOYPAD_H
