#include "Timer.h"

static inline int tima_shift(Memory& mem) {
    switch (mem.map[constants::TAC_REGISTER] & 3) {
        case 0b00: return 9;
        case 0b01: return 3;
        case 0b10: return 5;
        default:
            return 7;
    }
}

static inline int tima_shift(unsigned char tac_value) {
    switch (tac_value) {
        case 0b00: return 9;
        case 0b01: return 3;
        case 0b10: return 5;
        default:
            return 7;
    }
}

static inline int get_ticks_half_by_shift(int shift) {
    switch (shift) {
        case 9: return 1024/2;
        case 3: return 16/2;
        case 5: return 64/2;
        default:
            return 256/2;
    }
}

void Timer::tick(Memory& mem) {
    static unsigned char prev_bit = 0;
    static unsigned char cur_bit = 0;
    static bool tima_overflow = false;
    static unsigned char tac_prev = 0;
    static int overflow_counter = 0;
    static size_t current_t_cycle = 0;
    auto shift = tima_shift(mem);
    if (mem.div_written) {
        mem.div_written = false;
        prev_bit = (div_internal >> shift) & 1;
        cur_bit = 0;
        div_internal = 0;
        mem.div_internal_sync = 0;
        if (prev_bit && !cur_bit && (mem.map[constants::TAC_REGISTER] >> 2) & 1) {
            unsigned char prev = mem.map[constants::TIMA_REGISTER];
            current_t_cycle = 0;
            mem.map[constants::TIMA_REGISTER] = prev + 1;
            if (prev == 0xff) {
                tima_overflow = true;
                mem.tima_overflow = true;
            }
        }
        return;
    }
    ++current_t_cycle;
    prev_bit = std::exchange(cur_bit, (div_internal >> shift) & 1);
    ++div_internal;
    mem.map[constants::DIV_REGISTER] = ((div_internal >> 8) & 0xff);
    mem.div_internal_sync = div_internal;

    if (mem.tac_written) {
        // TODO: finish
//        std::cout << "TAC WRITTEN!!!!!\n";
//        auto pbit = ((div_internal >> tima_shift(tac_prev)) & 1);
//        if (pbit && !cur_bit){
//            tac_prev = mem.map[constants::TIMA_REGISTER] = mem.map[constants::TAC_REGISTER] + 1;
//            if (mem.map[constants::TAC_REGISTER] == 0xff) {
//                mem.tima_written = false;
//                tima_overflow = true;
//                mem.tima_overflow = true;
//            }
//        }
        mem.tac_written = false;
    }

    if ((mem.map[constants::TAC_REGISTER] >> 2) & 1) {
        if (tima_overflow) {
            if (overflow_counter++ != 4) return;
            overflow_counter = 0;
            tima_overflow = false;
            if (mem.tima_written) {
                mem.tima_written = false;
            } else {
                mem.map[constants::TIMA_REGISTER] = mem.map[constants::TMA_REGISTER];
                mem.map[constants::IF_ADDRESS] = (mem.map[constants::IF_ADDRESS] | (1 << 2));
            }
        } else if (prev_bit && !cur_bit) {
            current_t_cycle = 0;
            unsigned char prev = mem.map[constants::TIMA_REGISTER];
            mem.map[constants::TIMA_REGISTER] = prev + 1;
            if (prev == 0xff) {
                mem.tima_written = false;
                tima_overflow = true;
                overflow_counter= 0;
                mem.tima_overflow = true;
            }
        }
    }
}