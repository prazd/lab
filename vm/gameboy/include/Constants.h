#ifndef GAMEBOY_CONSTANTS_H
#define GAMEBOY_CONSTANTS_H
#include <iostream>

namespace constants {
    constexpr size_t DMA = 0xFF46;
    constexpr size_t BANK_ROM_SZ = 16384; // 16KiB
    constexpr size_t EXTERNAL_RAM_SZ = 8192; // 8KiB
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
    constexpr size_t IO_REGISTERS_BEGIN = 0xFF00;
    constexpr size_t IO_REGISTERS_END = 0xFF7F;
    constexpr size_t HRAM_BEGIN = 0xFF80;
    constexpr size_t HRAM_END = 0xFFFE;

    // Pallete

    constexpr size_t OBP0 = 0xFF48;
    constexpr size_t OBP1 = 0xFF49;

// https://gbdev.io/pandocs/Hardware_Reg_List.html?highlight=FF42#hardware-registers
// Hardware registers
    constexpr size_t LCDC_ADDRESS = 0xFF40;
    constexpr size_t STAT_ADDRESS = 0xFF41;
    constexpr size_t SCY_ADDRESS  =  0xFF42; // bg x
    constexpr size_t SCX_ADDRESS  =  0xFF43; // bg y
    constexpr size_t LY_ADDRESS   =   0xFF44; // scanline y
    constexpr size_t LYC_ADDRESS  =  0xFF45;
    constexpr size_t WY_ADDRESS   =   0xFF4A; // window x
    constexpr size_t WX_ADDRESS   =   0xFF4B; // window y

    constexpr size_t IF_ADDRESS   =   0xFF0F;
    constexpr size_t IE_ADDRESS   =   0xFFFF;

// Interrupts addresses
    constexpr size_t VBLANK_INTERRUPT = 0x40;
    constexpr size_t STAT_INTERRUPT   = 0x48;
    constexpr size_t TIMER_INTERRUPT  = 0x50;
    constexpr size_t SERIAL_INTERRUPT = 0x58;
    constexpr size_t JOYPAD_INTERRUPT = 0x60;

    constexpr size_t DIV_REGISTER  =  0xFF04;
    constexpr size_t TIMA_REGISTER =   0xFF05;
    constexpr size_t TMA_REGISTER  =  0xFF06;
    constexpr size_t TAC_REGISTER  =  0xFF07;
}

#endif //GAMEBOY_CONSTANTS_H
