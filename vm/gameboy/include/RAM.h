#ifndef GAMEBOY_RAM_H
#define GAMEBOY_RAM_H
#include "Constants.h"

struct RAM {
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
    unsigned char mem[0x10000]{}; // 64KiB

    enum MBC {
        MBC0,
        MBC1
    } current_mbc = MBC1;

    std::vector<char> rom{};
    size_t current_rom_bank = 1;
    std::vector<unsigned char> cart_ram{};
    size_t current_cart_ram = 0;

    unsigned char dummy_reg = 0xFF;

    bool dma_active = false;
    unsigned short dma_address = 0;
    unsigned int dma_counter = 0;
    unsigned char dma_t = 0;

    bool tima_written = false;
    bool div_written = false;
    bool glitch_overflow = false;
    bool tima_overflow = false;
    unsigned short div_internal_sync = 0;

    enum BankingMode {
        Simple,
        Advanced,
    } current_banking_mode = Simple;
    bool external_ram_enabled = false;

    void update_rom_bank01(size_t bank);
    void update_cart_ram(size_t n);
    bool lcd_enabled() const;
    void handle_mbc(unsigned short index, unsigned char value);
    void handle_mbc0(unsigned short index, unsigned char value);
    void handle_mbc1(unsigned short index, unsigned char value);


    // 0 - HBlank
    // 1 - VBlank
    // 2 - OAM
    // 3 - Drawing
    unsigned char get_ppu_mode() const;

    bool is_ram_avaliable(unsigned short index) const;
    void write(unsigned short index, unsigned char value);
    unsigned char read(unsigned short index) const;
};

#endif //GAMEBOY_RAM_H
