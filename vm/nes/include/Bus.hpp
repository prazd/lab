#ifndef NES_BUS_HPP
#define NES_BUS_HPP
#include "Controller.hpp"
struct Bus {
    // TODO:    
    enum Mapper{
        MMC1 = 0x1
    } current = MMC1;

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
    PPUREG ppustatus{};
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
