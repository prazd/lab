#ifndef NES_BUS_HPP
#define NES_BUS_HPP
#include "Controller.hpp"
#include "Mappers.hpp"
#include "GameGenie.hpp"
#include <Simple.hpp>
struct Bus {
    size_t master_clock = 0;
    enum StateDMA: unsigned char {
        Read,
        Write,
    } dma_state;
    static inline constexpr StateDMA next[2]{Write, Read};
    unsigned char dma_value{};
    unsigned char alignment{};
    inline void tick_dma(){
        if (alignment > 0){
            --alignment;
            return;
        }
        if (dma_state == Read){
            dma_value = read_prg(dma_address + dma_index);
        } else {
            oam[dma_index++] = dma_value;
            dma_active = (dma_index != 256);
        }
        dma_state = next[dma_state];
    }

    Mapper mapper;
#if GAME_GENIE_MODE
    GameGenie gg;
#endif
#if DEBUG_MODE
    static inline std::bitset<0x4000> ppu_write_set_addresses{};
    static inline std::bitset<0x4000> ppu_read_set_addresses{};
    static inline std::bitset<0x4000> ppu_trigger_read_addresses{};
    static inline std::bitset<0x4000> ppu_trigger_write_addresses{};

    SimpleDequeTC<std::pair<unsigned short,unsigned char>,3> ppu_write_addresses;
    bool ppu_addr_triggered_on_read = false;
    bool ppu_addr_triggered_on_write = false;

    static inline std::bitset<0x10000> cpu_write_set_addresses;
    static inline std::bitset<0x10000> cpu_read_set_addresses;
    static inline std::bitset<0x10000> cpu_trigger_write_addresses;
    static inline std::bitset<0x10000> cpu_trigger_read_addresses;

    SimpleDequeTC<std::pair<unsigned short,unsigned char>,5> cpu_write_addresses;
    bool cpu_addr_triggered_on_read = false;
    bool cpu_addr_triggered_on_write = false;
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
    bool ppu_irq_interrupt_flip_flop = false;
    bool apu_irq_interrupt_flip_flop = false;
    size_t open_bus_value_cycles = 0;
    unsigned char cpu_internal_bus_value{};
    unsigned char cpu_open_bus_value{};
    unsigned char ppu_open_bus_value{};
    unsigned short last_ppu_address = 0;
    unsigned short last_read_address = 0;

    size_t instructions_count = 0;
    size_t last_instruction_num_read = 0;

    enum Regs: unsigned char {
        PPUCTRL, PPUMASK, PPUSTATUS, PPUSCROLL, PPUADDR, PPUDATA,
        PPU_V, PPU_T, PPU_X
    };

    //  https://www.nesdev.org/wiki/PPU_registers
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

    bool dmc_dma_in_progress = false;
    unsigned char apufc{};
    bool apufc_requested = false;
    bool apu_enable_req = false;
    unsigned char apu_enable = 0;
    enum APUChan {Pulse1 = 0x1, Pulse2 = 0x2, Triangle = 0x4, Noise = 0x8, DMC = 0x10 };
    inline bool apu_is_enable(APUChan c){
        return apu_enable & c;
    }
    unsigned char apu_control = 0;
    bool apu_clear_frame_irq_req = false;
    unsigned short apu_dmc_bytes_remaining = 0;

    bool apu_pulse1_timer_update = false;
    bool apu_pulse1_envelope_update = false;
    bool apu_pulse1_start_flag = false;
    Controller c;
    bool latch_req = false;
    unsigned char latch_val = 0;

    void adjust_ppu_mirroring(unsigned short&);
    void adjust_cpu_mirroring(unsigned short&);
    void adjust_ppu_nt_mirroring(unsigned short& address);
    unsigned short get_nt_by_mirror_mode(Mapper::ScreenMirroring sm, unsigned char n);
    void tick();
    void controller_read_tick();
    void controller_write_tick();
    void write(unsigned short addr, unsigned char val);
    void unsafe_write(unsigned short addr, unsigned char val);
    unsigned char dmc_dma_read_prg(unsigned short addr);
    unsigned char read_prg(unsigned short addr);
    unsigned char read_prg_internal(unsigned short addr);
    unsigned char unsafe_read_prg(unsigned short addr);
    unsigned char read_chr(unsigned short addr);
    unsigned char unsafe_read_chr(unsigned short addr);
    void write_chr(unsigned short addr, unsigned char val);
    void unsafe_write_chr(unsigned short addr, unsigned char val);

    void open_bus_decay();
    void reset();
};
#endif //NES_BUS_HPP
