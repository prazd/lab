#include "Joypad.h"
#include "raylib.h"

void Joypad::update_state(Memory& mem) {
    unsigned char res = mem.read_unsafe(constants::JOYPAD);
    bool is_select_mode = !((res >> 5) & 1) && ((res >> 4) & 1);
    bool is_direction_mode = !((res >> 4) & 1) && ((res >> 5) & 1);
    if (!is_select_mode && !is_direction_mode){
        mem.write_unsafe(constants::JOYPAD, 0xff);
        return;
    }
    auto [pressed_res, interrupt] = current_pressed_values(is_direction_mode);
    res |= pressed_res;
    mem.write_unsafe(constants::JOYPAD, res);
    if (interrupt) mem.write_unsafe(constants::IF_ADDRESS, mem.read_unsafe(constants::IF_ADDRESS) | (1 << 4));
}

std::pair<unsigned char, bool> Joypad::current_pressed_values(bool is_direction) {
    int begin = is_direction ? 0 : 4;
    int end = is_direction ? 4 : 8;
    unsigned char result = 0xf;
    bool interrupt = false;
    for (int i = begin; i < end; ++i){
        if (IsKeyDown(keyboard_keys[i])){
            result &= ~(1 << key_shifts[i]);
        } else {
            bool current = result & (1 << key_shifts[i]);
            if (!current) interrupt = true;
        }
    }
    return {result, interrupt};
}
