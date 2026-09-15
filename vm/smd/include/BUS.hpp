#ifndef SMD_BUS_HPP
#define SMD_BUS_HPP
#include <cassert>
#include "Constants.hpp"
struct BUS {
    S_MODE unsigned char m68kmem[1 << 24]{};
    S_MODE unsigned char z80mem[1 << 16]{};
    enum MemSrc{M68Km, Z80m} memsrc = M68Km;
    enum Mode {UDS,LDS,UDS_LDS} mode = UDS_LDS;
    unsigned address{};
    unsigned short m68k_data{};
    unsigned char  z80_data{};
    unsigned char tick_count{};

    bool r_in_progress = false;
    bool w_in_progress = false;

    inline void m68k_r_req(unsigned address, Mode m){
        address = address;
        tick_count = 4;
        r_in_progress = true;
        memsrc = M68Km;
        mode = m;
    }
    inline void m68k_w_req(unsigned address, unsigned short data, Mode m){
#if DEBUG_MODE
        assert(!(address & 1));
#endif
        address = address;
        data = data;
        memsrc = M68Km;
        tick_count = 4;
        w_in_progress = true;
        mode = m;
    }
    inline void tick(){
        if (tick_count){
            --tick_count;
            // TODO: DTACK logic
            if (!tick_count) {
                if (memsrc == M68Km) {
                    if (r_in_progress) {
                        switch(mode) {
                            case UDS: {m68k_data &= 0xff;   m68k_data |= (m68kmem[address] << 8); break;}
                            case LDS: {m68k_data &= 0xff00; m68k_data |= m68kmem[address + 1]; break;}
                            case UDS_LDS: {
                                m68k_data = (m68kmem[address] << 8) | m68kmem[address + 1]; break;
                            }
                        }
                        r_in_progress = false;
                    } else if (w_in_progress) {
                        switch(mode) {
                            case UDS: {m68kmem[address] = (unsigned char)((m68k_data >> 8) & 0xff); break;}
                            case LDS: {m68kmem[address + 1] = (unsigned char)((m68k_data) & 0xff); break;}
                            case UDS_LDS:{
                                m68kmem[address] = (unsigned char)((m68k_data >> 8) & 0xff);
                                m68kmem[address + 1] = (unsigned char)((m68k_data) & 0xff);
                                break;
                            }
                        }
                        w_in_progress = false;
                    }
                }
            }
        }
    }
    inline unsigned short m68k_get() const {
        switch (mode) {
            case UDS: return (m68k_data >> 8) & 0xff;
            case LDS: return m68k_data & 0xff;
            default:
                return m68k_data;
        }
    }
};
#endif