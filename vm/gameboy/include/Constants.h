#ifndef GAMEBOY_CONSTANTS_H
#define GAMEBOY_CONSTANTS_H
#include <iostream>

namespace constants {
    constexpr size_t DMA = 0xFF46;
    constexpr size_t BANK_ROM_SZ = 16384;       // 16KiB
    constexpr size_t EXTERNAL_RAM_SZ = 8192;    // 8KiB
    constexpr size_t ECHO_RAM_BEGIN = 0xE000;
    constexpr size_t ECHO_RAM_END = 0xFDFF;
    constexpr size_t NOT_USABLE_BEGIN = 0xFEA0;
    constexpr size_t NOT_USABLE_END = 0xFEFF;
    constexpr size_t ROM_BANK_00_BEGIN = 0x0000;
    constexpr size_t ROM_BANK_00_END = 0x3FFF;
    constexpr size_t ROM_BANK_01_BEGIN = 0x4000;
    constexpr size_t ROM_BANK_01_END = 0x7FFF;
    constexpr size_t VRAM_BEGIN = 0x8000;
    constexpr size_t VRAM_END = 0x9FFF;
    constexpr size_t EXTERNAL_RAM_BEGIN = 0xA000;
    constexpr size_t EXTERNAL_RAM_END = 0xBFFF;
    constexpr size_t WRAM1_BEGIN = 0xC000;
    constexpr size_t WRAM1_END = 0xCFFF;
    constexpr size_t WRAM2_BEGIN = 0xD000;
    constexpr size_t WRAM2_END = 0xDFFF;
    constexpr size_t OAM_BEGIN = 0xFE00;
    constexpr size_t OAM_END = 0xFE9F;
    constexpr size_t IO_REGISTERS_END = 0xFF7F;
    constexpr size_t HRAM_BEGIN = 0xFF80;
    constexpr size_t HRAM_END = 0xFFFE;
    constexpr size_t JOYPAD = 0xFF00;

    // Pallete
    constexpr size_t BGP  = 0xFF47;
    constexpr size_t OBP0 = 0xFF48;
    constexpr size_t OBP1 = 0xFF49;

// https://gbdev.io/pandocs/Hardware_Reg_List.html?highlight=FF42#hardware-registers
// Hardware registers
    constexpr size_t LCDC_ADDRESS =   0xFF40;
    constexpr size_t STAT_ADDRESS =   0xFF41;
    constexpr size_t SCY_ADDRESS  =   0xFF42; // bg x
    constexpr size_t SCX_ADDRESS  =   0xFF43; // bg y
    constexpr size_t LY_ADDRESS   =   0xFF44; // scanline y
    constexpr size_t LYC_ADDRESS  =   0xFF45;
    constexpr size_t WY_ADDRESS   =   0xFF4A; // window x
    constexpr size_t WX_ADDRESS   =   0xFF4B; // window y

    constexpr size_t BG_TILE_MAP1 = 0x9800;
    constexpr size_t BG_TILE_MAP2 = 0x9C00;


    // Interrupts addresses
    constexpr size_t IF_ADDRESS   =   0xFF0F;
    constexpr size_t IE_ADDRESS   =   0xFFFF;

    constexpr size_t VBLANK_INTERRUPT = 0x40;
    constexpr size_t STAT_INTERRUPT   = 0x48;
    constexpr size_t TIMER_INTERRUPT  = 0x50;
    constexpr size_t SERIAL_INTERRUPT = 0x58;
    constexpr size_t JOYPAD_INTERRUPT = 0x60;

// Timer
    constexpr size_t DIV_REGISTER  =   0xFF04;
    constexpr size_t TIMA_REGISTER =   0xFF05;
    constexpr size_t TMA_REGISTER  =   0xFF06;
    constexpr size_t TAC_REGISTER  =   0xFF07;

// Serial
    constexpr size_t SB_ADDRESS = 0xFF01;
    constexpr size_t SC_ADDRESS = 0xFF02;

// Channels
    constexpr size_t NR10_ADDRESS = 0xFF10;
    constexpr size_t NR11_ADDRESS = 0xFF11;
    constexpr size_t NR12_ADDRESS = 0xFF12;
    constexpr size_t NR13_ADDRESS = 0xFF13;
    constexpr size_t NR14_ADDRESS = 0xFF14;

    constexpr size_t NR21_ADDRESS = 0xFF16;
    constexpr size_t NR22_ADDRESS = 0xFF17;
    constexpr size_t NR23_ADDRESS = 0xFF18;
    constexpr size_t NR24_ADDRESS = 0xFF19;

    constexpr size_t NR30_ADDRESS = 0xFF1A;
    constexpr size_t NR31_ADDRESS = 0xFF1B;
    constexpr size_t NR32_ADDRESS = 0xFF1C;
    constexpr size_t NR33_ADDRESS = 0xFF1D;
    constexpr size_t NR34_ADDRESS = 0xFF1E;

    constexpr size_t NR41_ADDRESS = 0xFF20;
    constexpr size_t NR42_ADDRESS = 0xFF21;
    constexpr size_t NR43_ADDRESS = 0xFF22;
    constexpr size_t NR44_ADDRESS = 0xFF23;

    constexpr size_t NR50_ADDRESS = 0xFF24;
    constexpr size_t NR51_ADDRESS = 0xFF25;
    constexpr size_t NR52_ADDRESS = 0xFF26;




}

#endif //GAMEBOY_CONSTANTS_H
