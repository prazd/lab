#include <iostream>
#include "raylib.h"

struct PPU {
    enum Mode {
        HBlank = 0b00, // H-Blank (Горизонтальная синхронизация)
        VBlank = 0b01, // V-Blank (Вертикальная синхронизация)
        OAM = 0b10,    // OAM Search (Поиск спрайтов)
        Drawing = 0b11 // Pixel Transfer (Передача пикселей)
    };
    Mode current_mode = OAM;
    
    const size_t LCDC_ADDRESS = 0xFF40;
    const size_t STAT_ADDRESS = 0xFF41;
    const size_t SCY_ADDRESS =  0xFF42;
    const size_t SCX_ADDRESS =  0xFF43;
    const size_t LY_ADDRESS =   0xFF44;
    const size_t LYC_ADDRESS =  0xFF45;
    const size_t WY_ADDRESS =   0xFF4A;
    const size_t WX_ADDRESS =   0xFF4B;

    bool lcd_enable = false;
    bool window_enable = false;
    bool obj_enable = false; // sprites
    bool bg_enable = false;

    template <typename RAM>
    void update_ppu_mode(RAM& ram) {
        ram.write(STAT_ADDRESS, (current_mode | ram.read(STAT_ADDRESS)));
    }

    template <typename RAM>
    void update_ly_lyc_compare_flag(RAM& ram) {
        /* set 2 bit of STAT_ADDRESS to LY == LYC */
    }
    
    template <typename RAM>
    void update_lcd(const RAM& ram) {
        lcd_enable = (((ram.read(LCDC_ADDRESS)) >> 7) & 1);
    }

    template <typename RAM>
    void update_window(const RAM& ram) {
        window_enable = (((ram.read(LCDC_ADDRESS)) >> 5) & 1);
    }

    template <typename RAM>
    void update_obj(const RAM& ram) {
        obj_enable = (((ram.read(LCDC_ADDRESS)) >> 1) & 1);
    }

    template <typename RAM>
    void update_bg(const RAM& ram) {
        bg_enable = ((ram.read(LCDC_ADDRESS)) & 1);
    }


    template <typename RAM>
    auto get_window_tile_map_area(const RAM& ram) {
        if (((ram.read(LCDC_ADDRESS) >> 6) & 1))
            return 0x9C00;
        return 0x9800;
    }

    template <typename RAM>
    auto get_tile_data_area(const RAM& ram) {
        if (((ram.read(LCDC_ADDRESS) >> 4) & 1))
            return 0x8800;
        return 0x8000;
    }
};

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

    const size_t STAT_ADDRESS = 0xFF41;

    const size_t BANK_ROM_SZ = 16384; // 16KiB
    const size_t EXTERNAL_RAM_SZ = 8192; // 8KiB
    
    const size_t ECHO_RAM_BEGIN = 0xE000;
    const size_t ECHO_RAM_END = 0xFDFF;
    const size_t NOT_USABLE_BEGIN = 0xFEA0;
    const size_t NOT_USABLE_END = 0xFEFF;
    const size_t ROM_BANK_00_BEGIN = 0x0000;
    const size_t ROM_BANK_00_END = 0x3FFF;
    const size_t ROM_BANK_01_BEGIN = 0x4000;
    const size_t ROM_BANK_01_END = 0x7FFF;
    const size_t VRAM_BEGIN = 0x8000;
    const size_t VRAM_END = 0x9FFF;
    const size_t EXTERNAL_RAM_BEGIN = 0xA000;
    const size_t EXTERNAL_RAM_END = 0xBFFF;
    const size_t WRAM1_BEGIN = 0xC000;
    const size_t WRAM1_END = 0xCFFF;
    const size_t WRAM2_BEGIN = 0xD000;
    const size_t WRAM2_END = 0xDFFF;
    const size_t OAM_BEGIN = 0xFE00;
    const size_t OAM_END = 0xFE9F;
    const size_t IO_REGISTERS_BEGIN = 0xFF00;
    const size_t IO_REGISTERS_END = 0xFF7F;
    const size_t HRAM_BEGIN = 0xFF80;
    const size_t HRAM_END = 0xFFFE;

    unsigned char mem[0x10000]{}; // 64KiB

    size_t current_rom_bank = 1;
    std::vector<unsigned char> rom{};
    void update_rom_bank01(size_t bank) {
        current_rom_bank = bank;
        memcpy(mem + BANK_ROM_SZ, rom.data() + (BANK_ROM_SZ * current_rom_bank), BANK_ROM_SZ);
    }
                   
    std::vector<unsigned char> cart_ram{};
    size_t current_cart_ram = 0;
    void update_cart_ram(size_t n) {
        current_cart_ram = n;
        memcpy(mem + 0xA000, cart_ram.data() + (EXTERNAL_RAM_SZ * current_cart_ram), EXTERNAL_RAM_SZ);
    }

    unsigned char dummy_reg = 0xFF;

    bool lcd_enabled() {
        return mem[0xFF00 + 0x40] & 0x80;
    }

    enum BankingMode {
        Simple, 
        Advanced,
    }
    current_banking_mode = Simple;
    
    bool external_ram_enabled = false;

    void handle_mbc(unsigned short index, unsigned char value) {
        // MBC-1
        if (index >= 0x0000 && index <= 0x1FFF) {
            external_ram_enabled = (value & 0x0f) == 0x0A;
        } else if (index >= 0x2000 && index <= ROM_BANK_00_END) {
            auto v = value & 0x1f;
            if (v == 0) v = 1;
            update_rom_bank01(v);
        } else if (index >= ROM_BANK_01_BEGIN && index <= 0x5FFF) {
            if (current_banking_mode == Simple) {
                auto h = value & 0x03;
                update_rom_bank01(h << 5 | current_rom_bank & 0x1f);
            } else {
                update_cart_ram(value & 0x03);
            }
        } else if (index >= 0x6000 && index <= ROM_BANK_01_END) {
            if (value & 1) current_banking_mode = Advanced;
            else current_banking_mode = Simple;
        }
    }

    PPU::Mode get_ppu_mode() {
        switch(mem[STAT_ADDRESS] & 0x03) {
            case 0b00: return PPU::Mode::HBlank;
            case 0b01: return PPU::Mode::VBlank;
            case 0b10: return PPU::Mode::OAM;
            default:
                return PPU::Mode::Drawing;
        }
    }

    bool is_ram_avaliable(unsigned short index) {
        auto current_ppu_mode = get_ppu_mode();
        if (((index >= VRAM_BEGIN && index <= VRAM_END)  && (current_ppu_mode == PPU::Drawing && lcd_enabled())) || 
            ((index >= EXTERNAL_RAM_BEGIN && index <= EXTERNAL_RAM_END)) && (cart_ram.empty() || !external_ram_enabled) ||
            ((index >= OAM_BEGIN && index <= OAM_END)) && ((current_ppu_mode == PPU::OAM || current_ppu_mode == PPU::Drawing) && lcd_enabled()) ||
            ((index >= NOT_USABLE_BEGIN && index <= NOT_USABLE_END))
        ) return false;
        return true;
    }

    void write(unsigned short index, unsigned char value){
        if (index <= ROM_BANK_01_END){
            handle_mbc(index, value);
            return;
        }
        if (index >= ECHO_RAM_BEGIN && index <= ECHO_RAM_END)
            index -= 0x2000;
        if (is_ram_avaliable(index))
            mem[index] = value;
    };

    unsigned char read(unsigned short index){
        if (index >= ECHO_RAM_BEGIN && index <= ECHO_RAM_END)
            index -= 0x2000;
        if (is_ram_avaliable(index))
            return mem[index];
        return dummy_reg;
    };
};

struct CPU {
    enum Register {A, F, B, C, D, E, H, L};
    enum Flag {Zf, Nf, Hf, Cf};
    unsigned char flags_pos[4] = {7,6,5,4};
    unsigned char regs[8]{};
    unsigned char rst_vec[8]{0x00,0x08,0x10,0x18,0x20,0x28,0x30,0x38};
    unsigned char ir{};
    unsigned char ie{};
    unsigned short pc{};
    unsigned short sp{};
    bool ime = false;

    enum Instruction {
        LD_0b01xxxyyy,
        LD_0b00xxx110,
        LD_0b01xxx110,
        LD_0b01110xxx,
        LD_0b00110110,
        LD_0b00001010,
        LD_0b00011010,
        LD_0b00000010,
        LD_0b00010010,
        LD_0b11111010,
        LD_0b11101010,
        LDH_0b11110010,
        LDH_0b11100010,
        LDH_0b11110000,
        LDH_0b11100000,
        LD_0b00111010,
        LD_0b00110010,
        LD_0b00101010,
        LD_0b00100010,
        LD_0b00xx0001,
        LD_0b00001000,
        LD_0b11111001,
        PUSH_0b11xx0101,
        POP_0b11xx0001,
        LD_0b11111000,
        ADD_0b10000xxx,
        ADD_0b10000110,
        ADD_0b11000110,
        ADC_0b10001xxx,
        ADC_0b10001110,
        ADC_0b11001110,
        SUB_0b10010xxx,
        SUB_0b10010110,
        SUB_0b11010110,
        SBC_0b10011xxx,
        SBC_0b10011110,
        SBC_0b11011110,
        CP_0b10111xxx,
        CP_0b10011110,
        CP_0b11111110,
        INC_0b00xxx100,
        INC_0b00110100,
        DEC_0b00xxx101,
        DEC_0b00110101,
        AND_0b10100xxx,
        AND_0b10100110,
        AND_0b11100110,
        OR_0b10110xxx,
        OR_0b10110110,
        OR_0b11110110,
        XOR_0b10101xxx,
        XOR_0b10101110,
        XOR_0b11101110,
        CCF_0b00111111,
        SCF_0b00110111,
        DAA_0b00100111,
        CPL_0b00101111,
        INC_0b00xx0011,
        DEC_0b00xx1011,
        ADD_0b00xx1001,
        ADD_0b11101000,

        RLCA_0b00000111,
        RRCA_0b00001111,
        RLA_0b00010111,
        RRA_0b00011111,

        RLC_0xCB_0b00000xxx,
        RRC_0xCB_0b00001xxx,
        RL_0xCB_0b00010xxx,
        RR_0xCB_0b00011xxx,
        SLA_0xCB_0b00100xxx,
        SRA_0xCB_0b00101xxx,
        SWAP_0xCB_0b00110xxx,
        SRL_0xCB_0b00111xxx,
        BIT_0xCB_0b01xxxxxx,
        RES_0xCB_0b10xxxxxx,
        SET_0xCB_0b11xxxxxx,
    
        
        // Control flow
        JP_0b11000011,
        JP_0b11101001,
        JP_0b110xx010,
        JR_0b00011000,
        JR_0b001xx000,
        CALL_0b11001101,
        CALL_0b110xx100,
        RET_0b11001001,
        RET_0b110xx000,
        RETI_0b11011001,
        RST_0b11xxx111,

        // Miscellaneous instructions
        HALT,
        STOP,
        DI_0b11110011,
        EI_0b11111011,
        NOP_0b00000000,
    };

    Register greg(unsigned char i) {
        switch(i){
            case 0b000: {return B;}
            case 0b001: {return C;}
            case 0b010: {return D;}
            case 0b011: {return E;}
            case 0b100: {return H;}
            case 0b101: {return L;}
            case 0b111: {return A;}
        }
        throw std::invalid_argument("Unknown register");
    }

    auto read3_5(unsigned char i) {
        return (0x7 & (i >> 3));
    }

    auto read0_2(unsigned char i) {
        return (0x7 & i);
    }

    void set_flag(Flag f, bool val) {
        if (val) {
            regs[F] |= (1 << flags_pos[f]);
        } else {
            regs[F] &= ~(1 << flags_pos[f]);
        }
    }

    unsigned char get_flag(Flag f) {
        return (regs[F] >> flags_pos[f]) & 1;
    }

    void decrement_pair(Register l, Register h) {
        unsigned short pair = ((regs[h] << 8) | regs[l]);
        --pair;
        regs[h] = ((pair >> 8) & 0xff);
        regs[l] = pair & 0xff;
    }

    void increment_pair(Register l, Register h) {
        unsigned short pair = ((regs[h] << 8) | regs[l]);
        ++pair;
        regs[h] = ((pair >> 8) & 0xff);
        regs[l] = pair & 0xff;
    }

    void update_pair_ld(unsigned char l,unsigned char h, unsigned char pair) {
        switch(pair) {
            case 0b00: {regs[C] = l; regs[B] = h; break;}
            case 0b01: {regs[E] = l; regs[D] = h; break;}
            case 0b10: {regs[L] = l; regs[H] = h; break;}
            case 0b11: {sp = ((h << 8) | l); break;}
        }

    }
    void update_pair_pop(unsigned char l, unsigned char h, unsigned char pair) {
        switch(pair) {
            case 0b00: {regs[C] = l; regs[B] = h; break;}
            case 0b01: {regs[E] = l; regs[D] = h; break;}
            case 0b10: {regs[L] = l; regs[H] = h; break;}
            case 0b11: {regs[F] = l & 0xf0; regs[A] = h; break;} // todo: check
        }
    }

    unsigned short getpair(unsigned char i) {
        switch(i) {
            case 0b00: {return  (regs[C] | (regs[B] << 8)); break;}
            case 0b01: {return  (regs[E] | (regs[D] << 8)); break;}
            case 0b10: {return  (regs[L] | (regs[H] << 8)); break;}
            case 0b11: {return sp; break;}
        }
        throw std::invalid_argument("Unknown register");
    }

    auto read45(unsigned char i) {
        return ((0b00110000 & i) >> 4);
    }

    auto read34(unsigned char i) {
        return ((0b0001'1000 & i) >> 3);
    }

    void add_reg_A(unsigned char val) {
        auto res = regs[A] + val;
        auto res8 = res & 0xff;
        set_flag(Nf, 0);
        set_flag(Zf, (res8 == 0));
        set_flag(Cf, (res > 0xff));
        set_flag(Hf, ((regs[A] & 0xf) + (val & 0xf) > 0xf));
        regs[A] = res8;
    }

    void add_reg_HL(unsigned short val) {
        unsigned short hl = (regs[H] << 8) | regs[L];
        unsigned int result = hl + val;
        set_flag(Nf, 0);
        set_flag(Cf, (result > 0xffff));
        set_flag(Hf, ((val & 0x0fff) + (hl & 0x0fff) > 0x0fff));
        regs[H] = (result >> 8) & 0xff;
        regs[L] = result & 0xff;
    }

    void sub_reg_A(unsigned char val) {
        auto res = (unsigned short)regs[A] - (unsigned short)val;
        auto res8 = res & 0xff;
        set_flag(Nf, 1);
        set_flag(Zf, (res8 == 0));
        set_flag(Cf, (val > regs[A]));
        set_flag(Hf, ((val & 0xf) > (regs[A] & 0xf)));
        regs[A] = res8;
    }

    void and_reg_A(unsigned char val) {
        regs[A] &= val;
        set_flag(Nf, 0);
        set_flag(Zf, (regs[A] == 0));
        set_flag(Cf, 0);
        set_flag(Hf, 1);
    }

     void or_reg_A(unsigned char val) {
        regs[A] |= val;
        set_flag(Nf, 0);
        set_flag(Zf, (regs[A] == 0));
        set_flag(Cf, 0);
        set_flag(Hf, 0);
    }

     void xor_reg_A(unsigned char val) {
        regs[A] ^= val;
        set_flag(Nf, 0);
        set_flag(Zf, (regs[A] == 0));
        set_flag(Cf, 0);
        set_flag(Hf, 0);
    }

    void sub_reg_A_flags_only(unsigned char val) {
        auto res = (unsigned short)regs[A] - (unsigned short)val;
        auto res8 = res & 0xff;
        set_flag(Nf, 1);
        set_flag(Zf, (res8 == 0));
        set_flag(Cf, (val > regs[A]));
        set_flag(Hf, ((val & 0xf) > (regs[A] & 0xf)));
    }

    void inc_reg(unsigned char i, RAM& ram) {
        auto reg = read_regs(i, ram);
        auto cp = reg;
        auto result = reg + 1;
        set_flag(Nf, 0);
        set_flag(Zf, result == 0);
        set_flag(Hf, (cp & 0xf) == 0xf);
        update_regs(i, ram, result);
    }

    void dec_reg(unsigned char i, RAM& ram) {
        auto reg = read_regs(i, ram);
        auto cp = reg;
        auto result = reg - 1;
        set_flag(Nf, 1);
        set_flag(Zf, result == 0);
        set_flag(Hf, (cp & 0xf) == 0);
        update_regs(i, ram, result);
    }

    unsigned short HL_data() {
        return ((regs[H] << 8) | regs[L]);
    }
    static constexpr unsigned char HL = 0b110;

    unsigned char read_regs(unsigned char i, RAM& ram) {
        switch(i){
            case 0b000: {return regs[B];}
            case 0b001: {return regs[C];}
            case 0b010: {return regs[D];}
            case 0b011: {return regs[E];}
            case 0b100: {return regs[H];}
            case 0b101: {return regs[L];}
            case 0b110: {return ram.read(HL_data());}
            case 0b111: {return regs[A];}
        }
        throw std::invalid_argument("Unknown register");
    }

    void update_regs(unsigned char i, RAM& ram, unsigned char value) {
        switch(i){
            case 0b000: {regs[B] = value; break;}
            case 0b001: {regs[C] = value; break;}
            case 0b010: {regs[D] = value; break;}
            case 0b011: {regs[E] = value; break;}
            case 0b100: {regs[H] = value; break;}
            case 0b101: {regs[L] = value; break;}
            case 0b110: {ram.write(HL_data(), value); break;}
            case 0b111: {regs[A] = value; break;}
        }
        throw std::invalid_argument("Unknown register");
    }

    void exec(unsigned char i, Instruction itype, RAM& ram) {
        switch(itype) {
            case LD_0b01xxxyyy:  { update_regs(read3_5(i), ram, read_regs(read0_2(i), ram)); ++pc; break;}
            case LD_0b01xxx110:  { update_regs(read3_5(i), ram, read_regs(HL, ram)); ++pc;  break;}
            case LD_0b00xxx110:  { update_regs(read3_5(i), ram, ram.read(pc + 1)); pc+=2; break;} 
            case LD_0b01110xxx:  { update_regs(HL, ram, read_regs(read0_2(i), ram)); ++pc; break;}
            case LD_0b00110110:  { update_regs(HL, ram, ram.read(pc + 1));  pc+=2; break;};
            case LD_0b00001010:  { regs[A] = ram.read(((regs[B] << 8)| regs[C])); ++pc; break;}
            case LD_0b00011010:  { regs[A] = ram.read(((regs[D] << 8)| regs[E])); ++pc; break;}
            case LD_0b00000010:  { ram.write(((regs[B] << 8)| regs[C]), regs[A]); ++pc; break;}
            case LD_0b00010010:  { ram.write(((regs[D] << 8)| regs[E]), regs[A]); ++pc; break;}
            case LD_0b11111010:  { regs[A] = ram.read(ram.read(pc + 1) | (ram.read(pc + 2) << 8)); pc+=3; break;}
            case LD_0b11101010:  { ram.write((ram.read(pc + 1) | (ram.read(pc + 2) << 8)), regs[A]); pc+=3; break;}
            case LDH_0b11110010: { regs[A] = ram.read(0xff00 | regs[C]); ++pc; break;}
            case LDH_0b11100010: { ram.write((0xff00 | regs[C]), regs[A]); ++pc; break;}
            case LDH_0b11110000: { regs[A] = ram.read(0xff00 | ram.read(pc+1)); pc+=2;  break;}
            case LDH_0b11100000: { ram.write((0xff00 | ram.read(pc+1)), regs[A]); pc+=2;  break;}
            case LD_0b00111010:  { regs[A] = read_regs(HL, ram); decrement_pair(L, H);  ++pc;  break;}
            case LD_0b00110010:  { update_regs(HL, ram, regs[A]); decrement_pair(L, H); ++pc; break;}
            case LD_0b00101010:  { regs[A] = read_regs(HL, ram); increment_pair(L, H);  ++pc;  break;}
            case LD_0b00100010:  { update_regs(HL, ram, regs[A]); increment_pair(L, H); ++pc; break;}
            case LD_0b00xx0001:  { update_pair_ld(ram.read(pc + 1), ram.read(pc+2), read45(i)); pc+=3; break;}
            case LD_0b00001000:  {
                auto addess = (ram.read(pc + 1) | (ram.read(pc + 2) << 8));
                ram.write(addess, sp & 0xff);
                ram.write(addess + 1, ((sp>>8) & 0xff));
                pc+=3;
                break;    
            }
            case LD_0b11111001: {sp = (regs[L] | (regs[H] << 8)); ++pc; break;} // SP = HL
            case PUSH_0b11xx0101: {
                auto val = getpair(read45(i)); 
                sp-=2;  
                ram.write(sp, ((val>>8) & 0xff));
                if (read45(i) == 0b11)
                    ram.write(sp+1, (val & 0xf0));
                else 
                    ram.write(sp+1, (val & 0xff));
                ++pc; 
                break;} // stack push xx
            case POP_0b11xx0001: {
                update_pair_pop(ram.read(sp+1), ram.read(sp), read45(i));
                sp+=2;
                ++pc;
                break;
            };
            case LD_0b11111000: {
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                auto e = static_cast<char>(ram.read(pc+1));
                unsigned int result = e + sp;
                set_flag(Cf, ((sp & 0xff) +  (unsigned char)(e) > 0xff));
                set_flag(Hf, ((sp & 0xf) + ((unsigned char)(e) & 0xf) > 0xf));
                regs[H] = ((result >> 8) & 0xff);
                regs[L] = (result & 0xff);
                pc+=2;
                break;
            }

            case ADD_0b10000xxx: { add_reg_A(read_regs(read0_2(i), ram)); ++pc; break;}
            case ADD_0b10000110: { add_reg_A(read_regs(HL, ram)); ++pc; break;}
            case ADD_0b11000110: { add_reg_A(ram.read(pc+1)); pc+=2; break;}
            case ADC_0b10001xxx: { add_reg_A(read_regs(read0_2(i), ram) + get_flag(Cf));++pc;break;}
            case ADC_0b10001110: { add_reg_A(read_regs(HL, ram) + get_flag(Cf)); ++pc; break;}
            case ADC_0b11001110: { add_reg_A(ram.read(pc+1) + get_flag(Cf)); pc+=2; break;}
            case SUB_0b10010xxx: { sub_reg_A(read_regs(read0_2(i), ram)); ++pc; break;}
            case SUB_0b10010110: { sub_reg_A(read_regs(HL, ram)); ++pc; break;}
            case SUB_0b11010110: { sub_reg_A(ram.read(pc+1)); pc+=2; break;}
            case SBC_0b10011xxx: { sub_reg_A(read_regs(read0_2(i), ram) + get_flag(Cf)); ++pc; break;} // (reg[A] - r - reg[C]) == reg[A] - (r + reg[C])
            case SBC_0b10011110: { sub_reg_A(read_regs(HL, ram) + get_flag(Cf)); ++pc; break;}
            case SBC_0b11011110: { sub_reg_A(ram.read(pc+1) + get_flag(Cf)); pc+=2; break;}
            case CP_0b10111xxx:  { sub_reg_A_flags_only(read_regs(read0_2(i), ram)); ++pc; break;}
            case CP_0b10011110:  { sub_reg_A_flags_only(read_regs(HL, ram)); ++pc; break;}
            case CP_0b11111110:  { sub_reg_A_flags_only(ram.read(pc+1)); pc+=2; break;}
            case INC_0b00xxx100: { inc_reg(read3_5(i), ram); ++pc; break;}
            case INC_0b00110100: { inc_reg(HL, ram); ++pc; break;}
            case DEC_0b00xxx101: { dec_reg(read3_5(i), ram); ++pc; break;}
            case DEC_0b00110101: { dec_reg(HL, ram); ++pc; break;}
            case AND_0b10100xxx: { and_reg_A(read_regs(read0_2(i), ram)); ++pc; break;}
            case AND_0b10100110: { and_reg_A(read_regs(HL, ram)); ++pc; break;}
            case AND_0b11100110: { and_reg_A(ram.read(pc+1)); pc+=2; break;}
            case OR_0b10110xxx:  { or_reg_A(read_regs(read0_2(i), ram)); ++pc; break;}
            case OR_0b10110110:  { or_reg_A(read_regs(HL, ram)); ++pc; break;}
            case OR_0b11110110:  { or_reg_A(ram.read(pc+1)); pc+=2; break;}
            case XOR_0b10101xxx: { xor_reg_A(read_regs(read0_2(i), ram)); ++pc; break;}
            case XOR_0b10101110: { xor_reg_A(read_regs(HL, ram)); ++pc; break;}
            case XOR_0b11101110: { xor_reg_A(ram.read(pc+1)); pc+=2; break;}
            case CCF_0b00111111: { set_flag(Cf, !get_flag(Cf)); set_flag(Nf, 0); set_flag(Hf, 0); ++pc; break;}
            case SCF_0b00110111: { set_flag(Cf, 1); set_flag(Nf, 0); set_flag(Hf, 0); ++pc; break;}
            case DAA_0b00100111: { /*todo*/ ++pc; break;}
            case CPL_0b00101111: {regs[A] = ~regs[A]; set_flag(Nf, 1); set_flag(Hf, 1); ++pc; break;}
            case INC_0b00xx0011: { 
                auto rr = read45(i);  
                unsigned short pair = getpair(rr); 
                ++pair;
                update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
                ++pc;
                break;
            }
            case DEC_0b00xx1011: {
                auto rr = read45(i);  
                unsigned short pair = getpair(rr); 
                --pair;
                update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
                ++pc;
                break;
            }
            case ADD_0b00xx1001: { add_reg_HL(getpair(read45(i))); ++pc; break;}
            case ADD_0b11101000: {
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                auto e = static_cast<char>(ram.read(pc+1));
                int result = e + sp;
                set_flag(Cf, ((sp & 0xff) +  (unsigned char)(e) > 0xff));
                set_flag(Hf, ((sp & 0xf) + ((unsigned char)(e) & 0xf) > 0xf));
                sp = (unsigned short)(result);
                pc+=2;
                break;
            }
            case RLCA_0b00000111: {
                auto res = std::rotl(regs[A], 1);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, regs[A] & 0x80);
                regs[A] = res;
                ++pc;  
                break;}
            case RRCA_0b00001111: {
                auto res = std::rotr(regs[A], 1);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, regs[A] & 1);
                regs[A] = res;
                ++pc;  
                break;}
            case RLA_0b00010111:  {
                auto carry = get_flag(Cf);
                auto res = ((regs[A] << 1) | carry);
                set_flag(Cf, regs[A] & 0x80);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                regs[A] = res;
                ++pc; break;}
            case RRA_0b00011111:  {
                auto carry = get_flag(Cf);
                auto res = ((regs[A] >> 1) | carry << 7);
                set_flag(Cf, regs[A] & 1);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                regs[A] = res;
                ++pc;  break;}
            case RLC_0xCB_0b00000xxx:  {
                auto rid = read0_2(ram.read(pc+1));
                auto reg = read_regs(rid, ram);
                auto res = std::rotl(reg, 1);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, reg & 0x80);
                update_regs(rid, ram, res);
                pc+=2; break;}
            case RRC_0xCB_0b00001xxx:  {
                auto rid = read0_2(ram.read(pc+1));
                auto reg = read_regs(rid, ram);
                auto res = std::rotr(reg, 1);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, reg & 1);
                update_regs(rid, ram, res);
                pc+=2; break;}
            case RL_0xCB_0b00010xxx:   {
                auto rid = read0_2(ram.read(pc+1));
                auto reg = read_regs(rid, ram);
                auto carry = get_flag(Cf);
                auto res = ((reg << 1) | carry);
                set_flag(Cf, reg & 0x80);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                update_regs(rid, ram, res);
                pc+=2; break;}
            case RR_0xCB_0b00011xxx:   {
                auto rid = read0_2(ram.read(pc+1));
                auto reg = read_regs(rid, ram);
                auto carry = get_flag(Cf);
                auto res = ((reg >> 1) | carry << 7);
                set_flag(Cf, reg & 1);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                update_regs(rid, ram, res);
                pc+=2; break;}
            case SLA_0xCB_0b00100xxx:  {
                auto rid = read0_2(ram.read(pc+1));
                auto reg = read_regs(rid, ram);
                auto res = reg << 1;
                set_flag(Cf, reg & 0x80);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                update_regs(rid, ram, res);
                pc+=2; break;}
            case SRA_0xCB_0b00101xxx:  {
                auto rid = read0_2(ram.read(pc+1));
                auto reg = read_regs(rid, ram);
                auto sign_bit = reg & 0x80;
                auto res = ((reg >> 1) | sign_bit);
                set_flag(Cf, reg & 1);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                update_regs(rid, ram, res);
                pc+=2; break;}
            case SWAP_0xCB_0b00110xxx: {
                auto rid = read0_2(ram.read(pc+1));
                auto reg = read_regs(rid, ram);
                auto l = reg & 0x0f;
                auto h = reg & 0xf0;
                auto res = (l << 4 | h >> 4);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, 0);
                update_regs(rid, ram, res);
                pc+=2; break;}
            case SRL_0xCB_0b00111xxx:  {
                auto rid = read0_2(ram.read(pc+1));
                auto reg = read_regs(rid, ram);
                auto res = reg >> 1;
                set_flag(Cf, reg & 1);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                update_regs(rid, ram, res);
                pc+=2; break;}
            case BIT_0xCB_0b01xxxxxx:  {
                auto bit_n = read3_5(ram.read(pc+1));
                auto reg = read_regs(read0_2(ram.read(pc+1)), ram);
                set_flag(Zf, !((reg >> bit_n) & 1));
                set_flag(Nf, 0);
                set_flag(Hf, 1);
                pc+=2; break;}
            case RES_0xCB_0b10xxxxxx:  {
                auto bit_n = read3_5(ram.read(pc+1));
                auto rid = read0_2(ram.read(pc+1));
                auto reg = read_regs(rid, ram);
                update_regs(rid, ram, reg & ~(1 << bit_n));
                pc+=2; break;}
            case SET_0xCB_0b11xxxxxx:  {
                auto bit_n = read3_5(ram.read(pc+1));
                auto rid = read0_2(ram.read(pc+1));
                auto reg = read_regs(rid, ram);
                update_regs(rid, ram, reg | (1 << bit_n));
                pc+=2; break;}

            // Control flow
            case JP_0b11000011: { pc = ((ram.read(pc+2) << 8) | ram.read(pc + 1)); break;}
            case JP_0b11101001: { pc = read_regs(HL, ram); break;}
            case JP_0b110xx010: { 
                auto val = read34(i);
                auto jmp = [&]{pc = ((ram.read(pc+2) << 8) | ram.read(pc + 1));};
                if (val == 0b00) {
                    if(!get_flag(Zf)) {jmp();}else{pc+=3;}
                } else if (val == 0b01) {
                    if(get_flag(Zf)) {jmp();}else{pc+=3;}
                } else if (val == 0b10) {
                    if(!get_flag(Cf)) {jmp();}else{pc+=3;}
                } else if (val == 0b11) {
                    if(get_flag(Cf)) {jmp();}else{pc+=3;}
                }
                break;
            }
            case JR_0b00011000: {pc += 2 + (char)(ram.read(pc+1)); break;}
            case JR_0b001xx000: {
                unsigned char val = read34(i);
                auto jmp = [&]{pc += 2 + (char)(ram.read(pc+1));};
                if (val == 0b00) {
                    if(!get_flag(Zf)) {jmp();}else{pc+= 2;}
                } else if (val == 0b01) {
                    if(get_flag(Zf)) {jmp();}else{pc+= 2;}
                } else if (val == 0b10) {
                    if(!get_flag(Cf)) {jmp();}else{pc+= 2;}
                } else if (val == 0b11) {
                    if(get_flag(Cf)) {jmp();}else{pc+= 2;}
                }
                break;
            }
            case CALL_0b11001101: {
                unsigned short nn = ((ram.read(pc + 2) << 8) | ram.read(pc + 1));
                sp-=2;
                ram.write(sp, (pc + 3) & 0xff);
                ram.write(sp + 1, ((pc + 3) >> 8));
                pc = nn;
                break;
            }
            case CALL_0b110xx100: {
                auto val = read34(i);
                unsigned short nn = ((ram.read(pc + 2) << 8) | ram.read(pc + 1));
                auto call = [&]{
                    sp-=2;
                    ram.write(sp, (pc + 3) & 0xff);
                    ram.write(sp+1, ((pc + 3) >> 8));
                    pc = nn;
                };
                if (val == 0b00) {
                    if(!get_flag(Zf)) {call();}else{pc+= 3;} 
                } else if (val == 0b01) {
                    if(get_flag(Zf)) {call();}else{pc+=3;}
                } else if (val == 0b10) {
                    if(!get_flag(Cf)) {call();}else{pc+=3;}
                } else if (val == 0b11) {
                    if(get_flag(Cf)) {call();}else{pc+=3;}
                }
                break;
            }
            case RET_0b11001001: { pc = ((ram.read(sp + 1) << 8) | ram.read(sp)); sp+=2; break;}
            case RET_0b110xx000: {
                auto val = read34(i);
                auto ret = [&]{
                    pc = ((ram.read(sp + 1) << 8) | ram.read(sp)); sp+=2;
                };
                if (val == 0b00) {
                    if(!get_flag(Zf)) {ret();}else{++pc;} 
                } else if (val == 0b01) {
                    if(get_flag(Zf)) {ret();}else{++pc;}
                } else if (val == 0b10) {
                    if(!get_flag(Cf)) {ret();}else{++pc;}
                } else if (val == 0b11) {
                    if(get_flag(Cf)) {ret();}else{++pc;}
                }
                break;
            }
            case RETI_0b11011001: {pc = ((ram.read(sp + 1) << 8) | ram.read(sp)); sp+=2; ime = true; break;}
            case RST_0b11xxx111: {
                unsigned short nn = ((0x0 << 8) | rst_vec[read3_5(i)]);
                sp-=2;
                ram.write(sp, (pc + 1) & 0xff);
                ram.write(sp + 1, ((pc + 1) >> 8));
                pc = nn;
                break;
            }

            case HALT: { /*todo*/ break;}
            case STOP: { /*todo*/ break;}
            case DI_0b11110011: {ime = false; ++pc; break;}
            case EI_0b11111011: {ime = true; ++pc; break;} // todo: check
            case NOP_0b00000000: {++pc; break;}
    }
    }
};


struct GameBoyVM{
    CPU cpu;
    RAM ram;
};

void draw_tiles(auto& arr) {
    std::vector<std::vector<int>> colors;
    for (size_t i = 0; i < 16; i+=2){
            auto l = arr[i];
            auto h = arr[i+1];
            std::vector<int> tmp{};
        for (int j = 0; j < 8; ++j) {
            auto lc = (l >> (7 - j)) & 1;
            auto hc = (h >> (7 - j)) & 1;
            tmp.push_back((hc << 1 | lc));
        }
        colors.push_back(tmp);
    }
    for (auto&& row: colors){
        for (auto el: row)
            std::cout << el;
        std::cout << '\n';
    }

    InitWindow(300, 300, "DrawTileTest");
    SetTargetFPS(60);
    auto get_color = [](int i){  
        switch(i){
            case 0: return WHITE;
            case 1: return GRAY;
            case 2: return DARKGRAY;
            default:
                return BLACK;
        }
    };
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(WHITE);
        float xstep = 300.f/8.f;
        float ystep = 300.f/8.f;
        float x = 0.0f;
        float y = 0.0f;
        for (auto&& row: colors){
            for (auto el: row) {
                DrawRectangle(x, y, xstep, ystep, get_color(el));
                x+=xstep;
            }
            y += ystep;
            x = 0.0f;
        }
        EndDrawing();
    }

    CloseWindow();
}

int main() {
    // test tiles draw
    unsigned char data[16]{0xFF, 0x00, 0x7E, 0xFF, 0x85, 0x81, 0x89, 0x83, 0x93, 0x85, 0xA5, 0x8B, 0xC9, 0x97, 0x7E, 0xFF};
    draw_tiles(data);
}
