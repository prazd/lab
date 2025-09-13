#include "Joypad.h"
#include "raylib.h"

void Joypad::update_state(RAM& ram) {
    unsigned char res = ram.read_unsafe(constants::JOYPAD);
    bool interrupt = false;
    auto keys_update = [&] (bool is_direction){
        int begin = is_direction ? 0 : 4;
        int end = is_direction ? 4 : 8;
        unsigned char result = 0xf;
        for (int i = begin; i < end; ++i){
            if (IsKeyDown(keyboard_keys[i])){
                result &= ~(1 << key_shifts[i]);
            } else {
                bool current = res & (1 << key_shifts[i]);
                if (!current) interrupt = true;
                result |= (1 << key_shifts[i]);
            }
        }
        return result;
    };

    if (!((res >> 5) & 1) && ((res >> 4) & 1)){          // Select mode
        res |= keys_update(false);
        ram.write_unsafe(constants::JOYPAD, res);
        if (interrupt) ram.write_unsafe(constants::IF_ADDRESS, ram.read_unsafe(constants::IF_ADDRESS) | (1 << 4));
    } else if (!((res >> 4) & 1) && ((res >> 5) & 1)) {  // Direction mode
        res |= keys_update(true);
        ram.write_unsafe(constants::JOYPAD, res);
        if (interrupt) ram.write_unsafe(constants::IF_ADDRESS, ram.read_unsafe(constants::IF_ADDRESS) | (1 << 4));
    } else {
        ram.write_unsafe(constants::JOYPAD, 0xff);
    }
}
