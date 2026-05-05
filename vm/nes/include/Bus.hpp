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
    VEC_MODE unsigned char oam[0x100]{};
    VEC_MODE unsigned char cpu_map[0x10000]{};
    VEC_MODE unsigned char ppu_map[0x4000]{};

    unsigned short dma_address{};
    bool dma_start = false;
    bool dma_active = false;
    unsigned dma_index = 0;
    bool nmi_interrupt_flip_flop = false;
    size_t status_read_cycle = 0;
    size_t current_nmi_cycle = 0;
    size_t current_ppu_cycle = 0;
    bool irq_interrupt_flip_flop = false;
    size_t open_bus_value_cycles = 0;
    unsigned char open_bus_value {};
    unsigned short last_ppu_address = 0;

    unsigned char ppustatus_buffer{};

    unsigned char ppuctrl{};
    unsigned char ppumask{};
    unsigned char ppustatus{};
    unsigned char oamaddr{};
    unsigned char oamdata{};
    unsigned char ppuscroll{};
    unsigned char ppuaddr{};
    unsigned char ppudata{};
    enum SpriteMode {
        m8x8,
        m8x16
    } sprite_mode = m8x8;
    unsigned short ppu_v{};
    unsigned short ppu_t{};
    bool ppu_w{};
    unsigned char ppu_x{};

    unsigned char apufc{};
    Controller c;

    void adjust_ppu_mirroring(unsigned short&);
    void adjust_cpu_mirroring(unsigned short&);
    void adjust_ppu_nt_mirroring(unsigned short& address);
    unsigned short get_nt_by_mirror_mode(Mapper::ScreenMirroring sm, unsigned char n);
    void tick();
    void write(unsigned short addr, unsigned char val);
    void unsafe_write(unsigned short addr, unsigned char val);
    unsigned char read_prg(unsigned short addr);
    unsigned char unsafe_read_prg(unsigned short addr);
    void write_chr(unsigned short addr, unsigned char val);
    unsigned char read_chr(unsigned short addr);
    void open_bus_decay();
    void reset();
};
#endif //NES_BUS_HPP
