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
            auto addr = oam_dma_address + dma_index;
            if (!((addr >= 0x4000 && addr <= 0x4019) && (last_arg_addr < 0x4000 || last_arg_addr > 0x4019)))
                dma_value = read_prg(oam_dma_address + dma_index);
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

    SimpleDequeTC<std::pair<unsigned short,unsigned char>,10> ppu_write_addresses;
    bool ppu_addr_triggered_on_read = false;
    bool ppu_addr_triggered_on_write = false;

    static inline std::bitset<0x10000> cpu_write_set_addresses;
    static inline std::bitset<0x10000> cpu_read_set_addresses;
    static inline std::bitset<0x10000> cpu_pc_read_set_addresses;
    static inline std::bitset<0x10000> cpu_trigger_write_addresses;
    static inline std::bitset<0x10000> cpu_trigger_read_addresses;

    SimpleDequeTC<std::pair<unsigned short,unsigned char>, 10> cpu_write_addresses;
    bool cpu_addr_triggered_on_read = false;
    bool cpu_addr_triggered_on_write = false;
#endif

    VEC_MODE unsigned char oam[0x100]{};
    VEC_MODE unsigned char cpu_map[0x10000]{};
    VEC_MODE unsigned char ppu_map[0x4000]{};

    enum FrameCounterMode {
        Step4, Step5
    } frame_counter_mode = Step5; // TODO: check

    bool apu_irq_inhibit = false;
    bool frame_counter_reset = false;
    size_t apu_current_tick = 0;
    size_t frame_counter_reset_ticks = 0;
    unsigned char ins_cycle = 0;

    unsigned short last_arg_addr{};
    bool next_write{};
    unsigned short oam_dma_address{};
    unsigned short dmc_dma_address = 0xc000;
    bool apu_enable_req{};
    char dmc_output_level{};
    size_t dmc_bytes_remaining = 0;
    bool dma_start = false;
    bool dma_active = false;
    unsigned dma_index = 0;
    bool nmi_interrupt_flip_flop = false;
    bool nmi_reject = false;
    size_t ppu_current_y = 0;
    size_t ppu_current_scanline = 0;
    size_t status_read_cycle = 0;
    size_t current_nmi_cycle = 0;
    size_t clear_nmi_cycle = 0;
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
    void inc_ppu_v_x();
    void inc_ppu_v_y();
    void inc_by_ppudata();
    unsigned short ppu_t{};
    bool ppu_w{};
    unsigned char ppu_x{};

    void apu_enable_handle();
    size_t dmc_rate_counter = 0;
    size_t dmc_rate_index = 0;
    size_t dmc_delay = 0;
    size_t dmc_dma_counter = 0;
    SimpleDequeTC<unsigned char, 1> dmc_sample_buffer{};
    bool dmc_need_sample_update = false;
    unsigned char dmc_shift_reg{};
    unsigned char dmc_shift_count{};

    size_t apu_fc_irq_set_cycle = 0;
    bool dmc_dma_in_progress = false;
    unsigned char apufc{};
    unsigned char apu_enable = 0;
    enum APUChan {Pulse1 = 0x1, Pulse2 = 0x2, Triangle = 0x4, Noise = 0x8, DMC = 0x10 };
    inline bool apu_is_enable(APUChan c){
        return apu_enable & c;
    }
    unsigned char apu_control = 0;
    bool apu_clear_frame_irq_req = false;

    bool apu_pulse1_timer_update = false;
    bool apu_pulse1_sweep_update = false;
    bool apu_pulse1_envelope_update = false;
    bool apu_pulse1_start_flag = false;

    bool apu_pulse2_timer_update = false;
    bool apu_pulse2_sweep_update = false;
    bool apu_pulse2_envelope_update = false;
    bool apu_pulse2_start_flag = false;

    bool apu_noise_start_flag = false;
    bool apu_noise_envelope_update = false;
    bool apu_noise_timer_update = false;

    bool apu_triangle_timer_update = false;
    bool apu_triangle_linear_counter_update = false;

    bool apu_noise_chan_loop_update = false;

    bool apu_dmc_dma_address_update{};


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

    static constexpr unsigned short ntsc_rates[] = {
            428,380,340,320,286,254,226,214,190,160,142,128,106,84,72,54
    };

    static constexpr unsigned char length_table[32]{
            10,
            254,
            20,
            2,
            40,
            4,
            80,
            6,
            160,
            8,
            60,
            10,
            14,
            12,
            26,
            14,
            12,
            16,
            24,
            18,
            48,
            20,
            96,
            22,
            192,
            24,
            72,
            26,
            16,
            28,
            32,
            30
    };
};
#endif //NES_BUS_HPP
