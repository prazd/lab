#ifndef NES_BUS_HPP
#define NES_BUS_HPP
#include "Controller.hpp"
#include "Mappers.h"
#include "GameGenie.h"
struct Bus {
    Mapper mapper;
#if GAME_GENIE_MODE
    GameGenie gg;
#endif
    unsigned char oam[0x100]{};
    unsigned short dma_address{};
    bool dma_active = false;
    unsigned dma_index = 0;
    bool nmi_interrupt_flip_flop = false;
    unsigned char current_nmi_cycle = 0;
    bool was_read_status = false;
    bool irq_interrupt_flip_flop = false;
    size_t open_bus_value_cycles = 0;
    unsigned char open_bus_value {};
    unsigned short last_ppu_address = 0;
    struct REG {
        unsigned char reg{};
        bool write_requested{};
        bool read_requested{};
    };
    REG ppuctrl{};
    REG ppumask{};
    REG ppustatus{};
    REG oamaddr{};
    REG oamdata{};
    REG ppuscroll{};
    REG ppuaddr{};
    REG ppudata{};

    REG apufc{};

    Controller c;

    void tick();
    void write(unsigned short addr, unsigned char val);
    void unsafe_write(unsigned short addr, unsigned char val);
    unsigned char read_prg(unsigned short addr);
    unsigned char unsafe_read_prg(unsigned short addr);
    unsigned char read_chr(unsigned short addr);
    void dma_tick(unsigned char val);
    void open_bus_decay();
    void reset();
};
#endif //NES_BUS_HPP
