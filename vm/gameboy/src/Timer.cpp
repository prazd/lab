#include "Timer.h"

static inline int tima_shift(RAM& ram) {
    switch (ram.read(constants::TAC_REGISTER) & 3) {
        case 0b00: return 9;
        case 0b01: return 3;
        case 0b10: return 5;
        default:
            return 7;
    }
}

void Timer::tick(RAM& ram) {
    static unsigned char prev_bit = 0;
    static unsigned char cur_bit = 0;
    static bool tima_overflow = false;
    static int overflow_counter = 0;
    auto shift = tima_shift(ram);
    if (ram.div_written) {
        ram.div_written = false;
        prev_bit = std::exchange(cur_bit, 0);
        div_internal = 0;
        ram.div_internal_sync = 0;
        if (prev_bit && !cur_bit && (ram.read(constants::TAC_REGISTER) >> 2) & 1) {
            unsigned char prev = ram.read(constants::TIMA_REGISTER);
            ram.write(constants::TIMA_REGISTER, prev + 1);
            if (prev == 0xff) {
                tima_overflow = true;
                ram.tima_overflow = true;
            }
        }
        return;
    }
    prev_bit = std::exchange(cur_bit, (div_internal >> shift) & 1);
    ++div_internal;
    ram.div_internal_sync = div_internal;
    if (ram.tima_written) {
        tima_overflow = ram.tima_overflow;
    }
    if ((ram.read(constants::TAC_REGISTER) >> 2) & 1) { // TIMA enabled
        if (tima_overflow || ram.glitch_overflow) {
            if (overflow_counter != 4) {
                ++overflow_counter;
                return;}
            overflow_counter = 0;
            tima_overflow = false;
            ram.tima_overflow = false;
            if (ram.glitch_overflow) ram.glitch_overflow = false;
            if (ram.tima_written){
                ram.tima_written = false;
            } else {
                ram.write(constants::IF_ADDRESS, ram.read(constants::IF_ADDRESS) | (1 << 2));
                ram.write(constants::TIMA_REGISTER, ram.read(constants::TMA_REGISTER));
            }
        }
        if (prev_bit && !cur_bit) {
            unsigned char prev = ram.read(constants::TIMA_REGISTER);
            ram.write(constants::TIMA_REGISTER, prev + 1);
            if (prev == 0xff) {
                tima_overflow = true;
                ram.tima_overflow = true;
            }
        }
    }
}