#ifndef NES_BUS_HPP
#define NES_BUS_HPP
#include "Controller.hpp"
#include "Mappers.h"
struct Bus {
    Mapper mapper;
    unsigned char oam[0x100]{};
    unsigned short dma_address{};
    bool dma_active = false;
    unsigned dma_index = 0;
    bool nmi_interrupt_flip_flop = false;

    struct PPUREG {
        unsigned char reg{};
        bool was_written{};
    };
    PPUREG ppuctrl{};
    PPUREG ppumask{};
    PPUREG ppustatus = {0xff, false}; // Enable Vblank for test
    PPUREG oamaddr{};
    PPUREG oamdata{};
    PPUREG ppuscroll{};
    PPUREG ppuaddr{};
    PPUREG ppudata{};
    Controller c;

    void write(unsigned short addr, unsigned char val);
    unsigned char read(unsigned short addr);
    unsigned char* read_ptr(unsigned short addr);
    void dma_tick(unsigned char val);
};
#endif //NES_BUS_HPP
