#ifndef SMD_BUS_HPP
#define SMD_BUS_HPP
#include "Constants.hpp"
struct BUS {
    S_MODE unsigned char mc68kmem[1 << 24]{};
    S_MODE unsigned char z80mem[1 << 16]{};
    enum Mode {UDS,LDS,UDS_LDS};
    Mode mc68k_mode = UDS_LDS;
    unsigned mc68k_address{};
    unsigned short mc68k_data{};
    unsigned char mc68k_tick_count{};
    unsigned char mc68k_w_sz{};
    bool mc68k_r_in_progress = false;
    bool mc68k_w_in_progress = false;
    inline void mc68k_r(unsigned address, Mode m){
        mc68k_address = address;
        mc68k_tick_count = 4;
        mc68k_r_in_progress = true;
        mc68k_mode = m;
    }
    inline void mc68k_w(unsigned address, unsigned short data, Mode m){
        mc68k_address = address;
        mc68k_data = data;
        mc68k_tick_count = 4;
        mc68k_w_in_progress = true;
        mc68k_mode = m;
    }
    inline void mc68k_tick(){
        if (mc68k_tick_count){
            --mc68k_tick_count;
            // TODO: DTACK logic
            if (!mc68k_tick_count) {
                if (mc68k_r_in_progress) {
                    switch(mc68k_mode) {
                        case UDS: {mc68k_data &= 0xff; mc68k_data |= (mc68kmem[mc68k_address] << 8); break;}
                        case LDS: {mc68k_data &= 0xff00; mc68k_data |= mc68kmem[mc68k_address + 1]; break;}
                        case UDS_LDS: {
                            mc68k_data = (mc68kmem[mc68k_address] << 8) | mc68kmem[mc68k_address + 1]; break;
                        }
                    }
                    mc68k_r_in_progress = false;
                } else if (mc68k_w_in_progress) {
                    switch(mc68k_mode) {
                        case UDS: {mc68kmem[mc68k_address] = (unsigned char)((mc68k_data >> 8) & 0xff); break;}
                        case LDS: {mc68kmem[mc68k_address + 1] = (unsigned char)((mc68k_data) & 0xff); break;}
                        case UDS_LDS:{
                            mc68kmem[mc68k_address] = (unsigned char)((mc68k_data >> 8) & 0xff);
                            mc68kmem[mc68k_address + 1] = (unsigned char)((mc68k_data) & 0xff);
                            break;
                        }
                    }
                    mc68k_w_in_progress = false;
                }
            }
        }
    }

};
#endif