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

static inline int get_ticks_by_shift(int shift) {
    switch (shift) {
        case 9: return 1024;
        case 3: return 16;
        case 5: return 64;
        default:
            return 256;
    }
}

void Timer::tick(Memory& mem) {
    static unsigned char prev_bit = 0;
    static unsigned char cur_bit = 0;
    static bool tima_overflow = false;
    static unsigned short prev_div = 0;
    static unsigned char tac_prev = 0xf8;
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
    }
    ++current_t_cycle;
    ++div_internal;
    prev_bit = std::exchange(cur_bit, (div_internal >> shift) & 1);
    mem.map[constants::DIV_REGISTER] = ((div_internal >> 8) & 0xff);
    mem.div_internal_sync = div_internal;

//    std::cout << "\n----------------\n";
//    std::cout << "Div:" << div_internal << '\n';
//    std::cout << " mem.map[constants::TIMA_REGISTER]:" <<  (unsigned)mem.map[constants::TIMA_REGISTER] << '\n';
//    std::cout << "Prev bit:" << (unsigned)prev_bit << '\n';
//    std::cout << "Cur bit:" << (unsigned)cur_bit << '\n';

    if (tima_overflow && overflow_counter == 0) {
        if (mem.tima_written)
            tima_overflow = false;
        else
            mem.tima_overflow = true;
    }

    if (mem.tima_overflow && overflow_counter == 4) {
        overflow_counter = 0;
        mem.tima_overflow = false;
        if (mem.tma_written){
            mem.map[constants::TIMA_REGISTER] = mem.map[constants::TMA_REGISTER];
            mem.tma_written = false;
        }
    }

    if (mem.tac_written) {
        bool old_enabled = (tac_prev >> 2) & 1;
        bool new_enabled = ((mem.map[constants::TAC_REGISTER] >> 2) & 1);
        bool old_bit = ((prev_div >> (tima_shift(tac_prev & 0x3))) & 1);
        if (((old_enabled && old_bit) && !(new_enabled && cur_bit))){
            current_t_cycle = 0;
            unsigned char prev = mem.map[constants::TIMA_REGISTER];
            mem.map[constants::TIMA_REGISTER] = prev + 1;
            if (prev == 0xff) {
                // todo: check - rappig toggle test
                mem.map[constants::TIMA_REGISTER] = mem.map[constants::TMA_REGISTER];
                mem.map[constants::IF_ADDRESS] = (mem.map[constants::IF_ADDRESS] | (1 << 2));
                mem.tma_written = false;
            }
        }
    }

    if ((mem.map[constants::TAC_REGISTER] >> 2) & 1) {
        if (tima_overflow) {
            if (++overflow_counter != 4) return;
            tima_overflow = false;
            mem.map[constants::TIMA_REGISTER] = mem.map[constants::TMA_REGISTER];
            mem.map[constants::IF_ADDRESS] = (mem.map[constants::IF_ADDRESS] | (1 << 2));
            mem.tma_written = false;
        } else if (prev_bit && !cur_bit) {
            current_t_cycle = 0;
            unsigned char prev = mem.map[constants::TIMA_REGISTER];
            mem.map[constants::TIMA_REGISTER] = prev + 1;
            if (prev == 0xff) {
                mem.tima_written = false;
                tima_overflow = true;
                overflow_counter= 0;
            }
        }
    }

    mem.tac_written = false;
    prev_div = div_internal;
    tac_prev = mem.map[constants::TAC_REGISTER];
}