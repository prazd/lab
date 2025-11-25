#ifndef GAMEBOY_RAM_H
#define GAMEBOY_RAM_H
#include "Constants.h"
#include <vector>
struct Memory {
    // 0xE000–0xFDFF: Echo RAM (Mirror of 0xC000–0xDDFF) Nintendo says use of this area is prohibited.
    // 0xFEA0–0xFEFF: Not Usable
    // 0x0000–0x3FFF: 16 KiB ROM Bank 00 (Fixed Bank)
    // 0x4000–0x7FFF: 16 KiB ROM Bank 01–NN (Switchable Bank)
    // 0x8000–0x9FFF: 8 KiB Video RAM (VRAM)
    // 0xA000–0xBFFF: 8 KiB External RAM
    // 0xC000–0xCFFF: 4 KiB Work RAM (WRAM)
    // 0xD000–0xDFFF: 4 KiB Work RAM (WRAM, Switchable in CGB)
    // 0xFE00–0xFE9F: Object Attribute Memory (OAM)
    // 0xFF00–0xFF7F: I/O Registers
    // 0xFF80–0xFFFE: High RAM (HRAM)
    unsigned char map[0x10000]{}; // 64KiB

    std::array<unsigned char, 0x100> cartridge_backup{};

    enum MBC {
        MBC0,
        MBC1,
        MBC2,
        MBC3,
        MBC5
    } current_mbc = MBC0;

    std::vector<char> rom{};
    size_t rom_sz = 0;
    int ram_sz = 0;
    size_t max_ram_bank_value = 0;
    size_t max_rom_bank_value = 0;
    size_t current_rom_bank0 = 0;
    size_t current_rom_bank1 = 1;

    size_t mbc_high_bits = 0;
    size_t mbc_low_bits = 1;

    std::vector<unsigned char> external_ram{};
    size_t current_external_ram_n = 0;
    size_t current_rom_bank_max = 0;

    unsigned char dummy_reg = 0xFF;

    bool dma_active = false;
    unsigned short dma_address = 0;
    unsigned int dma_counter = 0;
    unsigned char dma_t = 0;

    bool tima_written = false;
    bool div_written = false;
    bool tac_written = false;
    bool glitch_overflow = false;
    bool tima_overflow = false;
    bool tma_written = false;
    bool ppu_oam_usage = false;
    bool ch1_trigger = false;
    bool ch1_update_timer = false;
    bool ch1_enable_length_trigger = false;
    bool ch2_trigger = false;
    bool ch2_update_timer = false;
    bool ch2_enable_length_trigger = false;
    bool ch3_trigger = false;
    bool ch3_update_timer = false;
    bool ch3_enable_length_trigger = false;
    bool ch4_trigger = false;
    bool ch4_update_timer = false;
    bool ch4_enable_length_trigger = false;
    size_t ppu_internal_t_cycle = 0;
    unsigned short div_internal_sync = 0;

    unsigned short apu_regs[32] = {
        constants::NR10_ADDRESS,
        constants::NR11_ADDRESS,
        constants::NR12_ADDRESS,
        constants::NR13_ADDRESS,
        constants::NR14_ADDRESS,

        constants::NR20_ADDRESS,
        constants::NR21_ADDRESS,
        constants::NR22_ADDRESS,
        constants::NR23_ADDRESS,
        constants::NR24_ADDRESS,

        constants::NR30_ADDRESS,
        constants::NR31_ADDRESS,
        constants::NR32_ADDRESS,
        constants::NR33_ADDRESS,
        constants::NR34_ADDRESS,

        constants::NR40_ADDRESS,
        constants::NR41_ADDRESS,
        constants::NR42_ADDRESS,
        constants::NR43_ADDRESS,
        constants::NR44_ADDRESS,

        constants::NR50_ADDRESS,
        constants::NR51_ADDRESS,
        constants::NR52_ADDRESS,
        constants::NR53_ADDRESS,
        constants::NR54_ADDRESS,
        constants::NR55_ADDRESS,
        constants::NR56_ADDRESS,
        constants::NR57_ADDRESS,
        constants::NR58_ADDRESS,
        constants::NR59_ADDRESS,
        constants::NR60_ADDRESS,
        constants::NR61_ADDRESS,
    };

    unsigned short apu_regs_masks[32] = {
            0x80,
            0x3f,
            0x00,
            0xff,
            0xbf,

            0xff,
            0x3f,
            0x00,
            0xff,
            0xbf,

            0x7f,
            0xff,
            0x9f,
            0xff,
            0xbf,

            0xff,
            0xff,
            0x00,
            0x00,
            0xbf,

            0x00,
            0x00,
            0x70,

            0xff,
            0xff,
            0xff,
            0xff,
            0xff,
            0xff,
            0xff,
            0xff,
            0xff,
    };

#if DEBUG_MODE
    bool custom_ch1_active = true;
    bool custom_ch2_active = true;
    bool custom_ch3_active = true;
    bool custom_ch4_active = true;
#endif

    void apu_power_off();

    enum BankingMode {
        Simple,
        Advanced,
    } current_banking_mode = Simple;
    bool external_ram_enabled = false;
    bool lcd_enabled() const;
    void handle_mbc(unsigned short index, unsigned char value);
    void handle_mbc0(unsigned short index, unsigned char value);
    void handle_mbc1(unsigned short index, unsigned char value);
    void handle_mbc2(unsigned short index, unsigned char value);
    void handle_mbc3(unsigned short index, unsigned char value);
    void handle_mbc5(unsigned short index, unsigned char value);
    void dma_tick();


    // 0 - HBlank
    // 1 - VBlank
    // 2 - OAM
    // 3 - Drawing
    unsigned char get_ppu_mode() const;

    bool is_ram_avaliable(unsigned short index) const;

    void write(unsigned short index, unsigned char value);
    unsigned char read(unsigned short index) const;
    void write_unsafe(unsigned short index, unsigned char value);
    unsigned char read_unsafe(unsigned short index) const;

    void update_channels_triggers(unsigned short, unsigned char);
};

#endif //GAMEBOY_RAM_H
