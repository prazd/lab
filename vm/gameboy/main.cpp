#include <iostream>
#include "raylib.h"
#include <queue>
#include <array>

constexpr size_t STAT_ADDRESS = 0xFF41;
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

constexpr size_t LCDC_ADDRESS = 0xFF40;
constexpr size_t SCY_ADDRESS =  0xFF42;
constexpr size_t SCX_ADDRESS =  0xFF43;
constexpr size_t LY_ADDRESS =   0xFF44;
constexpr size_t LYC_ADDRESS =  0xFF45;
constexpr size_t WY_ADDRESS =   0xFF4A;
constexpr size_t WX_ADDRESS =   0xFF4B;


struct PPU {
    enum Mode {
        HBlank =  0b00,    // H-Blank (Горизонтальная синхронизация)
        VBlank =  0b01,    // V-Blank (Вертикальная синхронизация)
        OAM =     0b10,    // OAM Search (Поиск спрайтов)
        Drawing = 0b11     // Pixel Transfer (Передача пикселей)
    };
    Mode current_mode = OAM;

    Mode get_current_mode() const {
        return current_mode;
    }
    
    bool lcd_enable = true;
    bool window_enable = false;
    bool obj_enable = false; // sprites
    bool bg_enable = false;

    template <typename RAM>
    void set_ppu_mode(RAM& ram) {
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
    void update_regs(RAM& ram) {
        update_lcd(ram);
        update_window(ram);
        update_obj(ram);
        update_bg(ram);
    }
    
    template <typename RAM>
    auto get_obj_size(const RAM& ram) {
        return (((ram.read(LCDC_ADDRESS)) >> 2) & 1) ? 8 : 16;
    }

    template <typename RAM>
    auto get_window_tile_map_area(const RAM& ram) {
        if (((ram.read(LCDC_ADDRESS) >> 6) & 1))
            return 0x9C00;
        return 0x9800;
    }

    template <typename RAM>
    auto get_bg_tile_map_area(const RAM& ram) {
        if (((ram.read(LCDC_ADDRESS) >> 3) & 1))
            return 0x9C00;
        return 0x9800;
    }

    template <typename RAM>
    auto get_tile_data_area(const RAM& ram) {
        if (((ram.read(LCDC_ADDRESS) >> 4) & 1))
            return 0x8800;
        return 0x8000;
    }

    struct Sprite {
        unsigned char y_pos{};
        unsigned char x_pos{};
        unsigned char index_tile{};
        unsigned char attrib{};
    };

    template <typename RAM>
    auto oam_scan(RAM& ram) {
        auto ly = ram.read(LY_ADDRESS);
        std::array<Sprite, 10> result{};
        size_t count{};
        auto obj_sz = get_obj_size(ram);
        for (int i = 0; i < 160; i+=4){
            auto val = ram.read(OAM_BEGIN + i);
            auto top = val - 16;
            if (ly >= top && ly < top + obj_sz){
                result[count++] = Sprite{
                    .y_pos = val,
                    .x_pos = ram.read(OAM_BEGIN + i + 1),
                    .index_tile = ram.read(OAM_BEGIN + i + 2),
                    .attrib = ram.read(OAM_BEGIN + i + 3),
                };
            }
            if (count == 10) break;
        }
        std::sort(result.begin(), result.begin() + count, [](auto& l, auto& r){return l.x_pos < r.x_pos;});
        return result;
    }

    auto tile_data_to_pixels(unsigned short tile_data) {
        unsigned char l = tile_data & 0xff;
        unsigned char h = (tile_data >> 8) & 0xff;
        std::array<unsigned char, 8> pixels{};
        for (int i = 0; i < 8; ++i) {
            auto lc = (l >> (7 - i)) & 1;
            auto hc = (h >> (7 - i)) & 1;
            pixels[i] = ((hc << 1 | lc));
        }
        return pixels;
    }

    std::queue<int> bg_fifo{};    // loaded with background/window data pixels
    std::queue<int> sprite_fifo{};

    template <typename RAM>
    void execute(RAM& ram) {
        if (mode_finished) return;
        switch (current_mode) {
            case OAM:     {oam_scan(ram); break;}
            case VBlank:  {/*todo*/ break;}
            case Drawing: {/*todo*/ break;}
            case HBlank:  {/*todo*/ break;}
        }
        mode_finished = true;
    }

    template <typename RAM>
    void draw(const std::array<Sprite, 10>& sprites, RAM& ram) {
        /*todo*/
    }

    int x_pixel = {};    // count of drawed pixel in line
    int current_line = {};
    int current_t_cycle_count = {};
    bool longest_line = false;
    bool mode_finished = false;

    template <typename RAM>
    void handle_t_cycle(int t_cycle, RAM& ram) {
        update_regs(ram);
        current_t_cycle_count += t_cycle;
        // write_memory(0xFF44, current_line); // Update LY - todo
        if (current_mode == OAM && current_t_cycle_count >= 80){ // maybe handle
            current_mode = Drawing;
            mode_finished = false;
            set_ppu_mode(ram);
        } else if (current_mode == Drawing && ((longest_line && current_t_cycle_count >= 289) || (!longest_line && current_t_cycle_count >= 172))) { 
            current_mode = HBlank;
            mode_finished = false;
            set_ppu_mode(ram);
        } else if (current_mode == HBlank && ((!longest_line && current_t_cycle_count >= 204) || (longest_line && current_t_cycle_count >= 87))) {
            ++current_line;
            current_t_cycle_count = 0;
            if (current_line == 144)
                current_mode = VBlank;
                // request_interrupt(VBLANK_INTERRUPT); // todo:
            else
                current_mode = OAM;
            mode_finished = false;
            set_ppu_mode(ram);
        } else if (current_mode == VBlank && current_t_cycle_count >= 4560) {
            current_mode = OAM;
            current_line = 0;
            current_t_cycle_count = 0;
            mode_finished = false;
            set_ppu_mode(ram);
        }

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
        // https://hacktix.github.io/GBEDG/mbcs/mbc1/
        if (index >= 0x0000 && index <= 0x1FFF) {
            external_ram_enabled = (value & 0xf) == 0x0A;
        } else if (index >= 0x2000 && index <= ROM_BANK_00_END) {
            auto v = value & 0x1f;
            if (v == 0) v = 1;
            update_rom_bank01(v);
        } else if (index >= ROM_BANK_01_BEGIN && index <= 0x5FFF) {
            auto v = value & 0x3;
            if (current_banking_mode == Simple) {
                update_rom_bank01(v << 5 | current_rom_bank & 0x1f);
            } else {
                update_cart_ram(v);
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
            ((index >= EXTERNAL_RAM_BEGIN && index <= EXTERNAL_RAM_END)) && (cart_ram.empty() || !external_ram_enabled) || // todo: check external RAM 
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



    int t_cycle_by_ins(Instruction ins, unsigned char value = 0) { // value - 0xCB pc + 1 / cc_flag
        switch(ins) {     
            case LD_0b01xxxyyy:       {return 1;}
            case LD_0b00xxx110:       {return 2;}
            case LD_0b01xxx110:       {return 2;}
            case LD_0b01110xxx:       {return 2;}
            case LD_0b00110110:       {return 3;}
            case LD_0b00001010:       {return 2;}
            case LD_0b00011010:       {return 2;}
            case LD_0b00000010:       {return 2;}
            case LD_0b00010010:       {return 2;}
            case LD_0b11111010:       {return 4;}
            case LD_0b11101010:       {return 4;}
            case LDH_0b11110010:      {return 2;}
            case LDH_0b11100010:      {return 2;}
            case LDH_0b11110000:      {return 3;}
            case LDH_0b11100000:      {return 3;}
            case LD_0b00111010:       {return 2;}
            case LD_0b00110010:       {return 2;}
            case LD_0b00101010:       {return 2;}
            case LD_0b00100010:       {return 2;}
            case LD_0b00xx0001:       {return 3;}
            case LD_0b00001000:       {return 5;}
            case LD_0b11111001:       {return 2;}
            case PUSH_0b11xx0101:     {return 4;}
            case POP_0b11xx0001:      {return 3;}
            case LD_0b11111000:       {return 3;}
            case ADD_0b10000xxx:      {return 1;}
            case ADD_0b10000110:      {return 2;}
            case ADD_0b11000110:      {return 2;}
            case ADC_0b10001xxx:      {return 1;}
            case ADC_0b10001110:      {return 2;}
            case ADC_0b11001110:      {return 2;}
            case SUB_0b10010xxx:      {return 1;}
            case SUB_0b10010110:      {return 2;}
            case SUB_0b11010110:      {return 2;}
            case SBC_0b10011xxx:      {return 1;}
            case SBC_0b10011110:      {return 2;}
            case SBC_0b11011110:      {return 2;}
            case CP_0b10111xxx:       {return 1;}
            case CP_0b10011110:       {return 2;}
            case CP_0b11111110:       {return 2;}
            case INC_0b00xxx100:      {return 1;}
            case INC_0b00110100:      {return 3;}
            case DEC_0b00xxx101:      {return 1;}
            case DEC_0b00110101:      {return 3;}
            case AND_0b10100xxx:      {return 1;}
            case AND_0b10100110:      {return 2;}
            case AND_0b11100110:      {return 2;}
            case OR_0b10110xxx:       {return 1;}
            case OR_0b10110110:       {return 2;}
            case OR_0b11110110:       {return 2;}
            case XOR_0b10101xxx:      {return 1;}
            case XOR_0b10101110:      {return 2;}
            case XOR_0b11101110:      {return 2;}
            case CCF_0b00111111:      {return 1;}
            case SCF_0b00110111:      {return 1;}
            case DAA_0b00100111:      {return 1;}
            case CPL_0b00101111:      {return 1;}
            case INC_0b00xx0011:      {return 2;}
            case DEC_0b00xx1011:      {return 2;}
            case ADD_0b00xx1001:      {return 2;}
            case ADD_0b11101000:      {return 4;}
            case RLCA_0b00000111:     {return 1;}
            case RRCA_0b00001111:     {return 1;}
            case RLA_0b00010111:      {return 1;}
            case RRA_0b00011111:      {return 1;}

            case RLC_0xCB_0b00000xxx: {return value == 0x06 ? 4 : 2;} // RLC (HL): Rotate left circular (indirect HL) - 4 (0x06)
            case RRC_0xCB_0b00001xxx: {return value == 0x0E ? 4 : 2;} // RRC (HL): Rotate right circular (indirect HL) - 4 (0x0E)
            case RL_0xCB_0b00010xxx:  {return value == 0x16 ? 4 : 2;} // RL (HL): Rotate left (indirect HL) - 4 (0x16)
            case RR_0xCB_0b00011xxx:  {return value == 0x1E ? 4 : 2;} // RR (HL): Rotate right (indirect HL) - 4 (0x1E)
            case SLA_0xCB_0b00100xxx: {return value == 0x26 ? 4 : 2;} // SLA (HL): Shift left arithmetic (indirect HL) - 4 (0x26)
            case SRA_0xCB_0b00101xxx: {return value == 0x2E ? 4 : 2;} // SRA (HL): Shift right arithmetic (indirect HL) - 4 (0x2E)
            case SWAP_0xCB_0b00110xxx:{return value == 0x36 ? 4 : 2;} // SWAP (HL): Swap nibbles (indirect HL) - 4 (0x36)
            case SRL_0xCB_0b00111xxx: {return value == 0x3E ? 4 : 2;} // SRL (HL): Shift right logical (indirect HL) - 4 (0x3E)
            case BIT_0xCB_0b01xxxxxx: {return (value & 0x07) == 6 ? 3 : 2;} // BIT b, (HL): Test bit (indirect HL) - 3(0b01xxx110)
            case RES_0xCB_0b10xxxxxx: {return (value & 0x07) == 6 ? 4 : 2;} // RES b, (HL): Reset bit (indirect HL) - 4(0b10xxx110)
            case SET_0xCB_0b11xxxxxx: {return (value & 0x07) == 6 ? 4 : 2;} // SET b, (HL): Set bit (indirect HL) - 4 (0b11xxx110)

            case JP_0b11000011:       {return 4;}
            case JP_0b11101001:       {return 1;}
            case JP_0b110xx010:       {return value ? 4 : 3;} // n 4 machine cycles (cc=true) / 3 machine cycles (cc=false)
            case JR_0b00011000:       {return 3;}
            case JR_0b001xx000:       {return value ? 3 : 2;} // 3 machine cycles (cc=true)/ 2 machine cycles (cc=false)
            case CALL_0b11001101:     {return 6;} 
            case CALL_0b110xx100:     {return value ? 6 : 3;} // 6 machine cycles (cc=true) / 3 machine cycles (cc=false)
            case RET_0b11001001:      {return 4;} 
            case RET_0b110xx000:      {return value ? 5 : 2;} // n 5 machine cycles (cc=true)/2 machine cycles (cc=false)
            case RETI_0b11011001:     {return 4;}
            case RST_0b11xxx111:      {return 4;}
          
            case HALT:                {return 0;}
            case STOP:                {return 0;}
            case DI_0b11110011:       {return 1;}
            case EI_0b11111011:       {return 1;}
            case NOP_0b00000000:      {return 1;}
        }
    }

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

    bool get_cc_flag(auto val) {
        return (val == 0b00 && !get_flag(Zf) ||
                     val == 0b01 &&  get_flag(Zf) || 
                     val == 0b10 && !get_flag(Cf) || 
                     val == 0b11 && get_flag(Cf));
    }

    unsigned char fetch(RAM& ram) {
        return ram.read(pc);
    }

    Instruction decode(unsigned char ins) {
        /*todo*/
        return ADD_0b00xx1001;
    }

    int execute(unsigned char i, Instruction itype, RAM& ram) {
        switch(itype) {
            case LD_0b01xxxyyy:  { update_regs(read3_5(i), ram, read_regs(read0_2(i), ram)); ++pc; return t_cycle_by_ins(itype);}
            case LD_0b01xxx110:  { update_regs(read3_5(i), ram, read_regs(HL, ram)); ++pc;  return t_cycle_by_ins(itype);}
            case LD_0b00xxx110:  { update_regs(read3_5(i), ram, ram.read(pc + 1)); pc+=2; return t_cycle_by_ins(itype);} 
            case LD_0b01110xxx:  { update_regs(HL, ram, read_regs(read0_2(i), ram)); ++pc; return t_cycle_by_ins(itype);}
            case LD_0b00110110:  { update_regs(HL, ram, ram.read(pc + 1));  pc+=2; return t_cycle_by_ins(itype);};
            case LD_0b00001010:  { regs[A] = ram.read(((regs[B] << 8)| regs[C])); ++pc; return t_cycle_by_ins(itype);}
            case LD_0b00011010:  { regs[A] = ram.read(((regs[D] << 8)| regs[E])); ++pc; return t_cycle_by_ins(itype);}
            case LD_0b00000010:  { ram.write(((regs[B] << 8)| regs[C]), regs[A]); ++pc; return t_cycle_by_ins(itype);}
            case LD_0b00010010:  { ram.write(((regs[D] << 8)| regs[E]), regs[A]); ++pc; return t_cycle_by_ins(itype);}
            case LD_0b11111010:  { regs[A] = ram.read(ram.read(pc + 1) | (ram.read(pc + 2) << 8)); pc+=3; return t_cycle_by_ins(itype);}
            case LD_0b11101010:  { ram.write((ram.read(pc + 1) | (ram.read(pc + 2) << 8)), regs[A]); pc+=3; return t_cycle_by_ins(itype);}
            case LDH_0b11110010: { regs[A] = ram.read(0xff00 | regs[C]); ++pc; return t_cycle_by_ins(itype);}
            case LDH_0b11100010: { ram.write((0xff00 | regs[C]), regs[A]); ++pc; return t_cycle_by_ins(itype);}
            case LDH_0b11110000: { regs[A] = ram.read(0xff00 | ram.read(pc+1)); pc+=2;  return t_cycle_by_ins(itype);}
            case LDH_0b11100000: { ram.write((0xff00 | ram.read(pc+1)), regs[A]); pc+=2;  return t_cycle_by_ins(itype);}
            case LD_0b00111010:  { regs[A] = read_regs(HL, ram); decrement_pair(L, H);  ++pc;  return t_cycle_by_ins(itype);}
            case LD_0b00110010:  { update_regs(HL, ram, regs[A]); decrement_pair(L, H); ++pc; return t_cycle_by_ins(itype);}
            case LD_0b00101010:  { regs[A] = read_regs(HL, ram); increment_pair(L, H);  ++pc;  return t_cycle_by_ins(itype);}
            case LD_0b00100010:  { update_regs(HL, ram, regs[A]); increment_pair(L, H); ++pc; return t_cycle_by_ins(itype);}
            case LD_0b00xx0001:  { update_pair_ld(ram.read(pc + 1), ram.read(pc+2), read45(i)); pc+=3; return t_cycle_by_ins(itype);}
            case LD_0b00001000:  {
                auto addess = (ram.read(pc + 1) | (ram.read(pc + 2) << 8));
                ram.write(addess, sp & 0xff);
                ram.write(addess + 1, ((sp>>8) & 0xff));
                pc+=3;
                return t_cycle_by_ins(itype);    
            }
            case LD_0b11111001: {sp = (regs[L] | (regs[H] << 8)); ++pc; return t_cycle_by_ins(itype);} // SP = HL
            case PUSH_0b11xx0101: {
                auto val = getpair(read45(i)); 
                sp-=2;  
                ram.write(sp, ((val>>8) & 0xff));
                if (read45(i) == 0b11)
                    ram.write(sp+1, (val & 0xf0));
                else 
                    ram.write(sp+1, (val & 0xff));
                ++pc; 
                return t_cycle_by_ins(itype);} // stack push xx
            case POP_0b11xx0001: {
                update_pair_pop(ram.read(sp+1), ram.read(sp), read45(i));
                sp+=2;
                ++pc;
                return t_cycle_by_ins(itype);
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
                return t_cycle_by_ins(itype);
            }

            case ADD_0b10000xxx: { add_reg_A(read_regs(read0_2(i), ram)); ++pc; return t_cycle_by_ins(itype);}
            case ADD_0b10000110: { add_reg_A(read_regs(HL, ram)); ++pc;  return t_cycle_by_ins(itype);}
            case ADD_0b11000110: { add_reg_A(ram.read(pc+1)); pc+=2; return t_cycle_by_ins(itype);}
            case ADC_0b10001xxx: { add_reg_A(read_regs(read0_2(i), ram) + get_flag(Cf)); ++pc; return t_cycle_by_ins(itype);}
            case ADC_0b10001110: { add_reg_A(read_regs(HL, ram) + get_flag(Cf)); ++pc; return t_cycle_by_ins(itype);}
            case ADC_0b11001110: { add_reg_A(ram.read(pc+1) + get_flag(Cf)); pc+=2; return t_cycle_by_ins(itype);}
            case SUB_0b10010xxx: { sub_reg_A(read_regs(read0_2(i), ram)); ++pc; return t_cycle_by_ins(itype);}
            case SUB_0b10010110: { sub_reg_A(read_regs(HL, ram)); ++pc; return t_cycle_by_ins(itype);}
            case SUB_0b11010110: { sub_reg_A(ram.read(pc+1)); pc+=2; return t_cycle_by_ins(itype);}
            case SBC_0b10011xxx: { sub_reg_A(read_regs(read0_2(i), ram) + get_flag(Cf)); ++pc; return t_cycle_by_ins(itype);} // (reg[A] - r - reg[C]) == reg[A] - (r + reg[C])
            case SBC_0b10011110: { sub_reg_A(read_regs(HL, ram) + get_flag(Cf)); ++pc; return t_cycle_by_ins(itype);}
            case SBC_0b11011110: { sub_reg_A(ram.read(pc+1) + get_flag(Cf)); pc+=2; return t_cycle_by_ins(itype);}
            case CP_0b10111xxx:  { sub_reg_A_flags_only(read_regs(read0_2(i), ram)); ++pc; return t_cycle_by_ins(itype);}
            case CP_0b10011110:  { sub_reg_A_flags_only(read_regs(HL, ram)); ++pc; return t_cycle_by_ins(itype);}
            case CP_0b11111110:  { sub_reg_A_flags_only(ram.read(pc+1)); pc+=2; return t_cycle_by_ins(itype);}
            case INC_0b00xxx100: { inc_reg(read3_5(i), ram); ++pc; return t_cycle_by_ins(itype);}
            case INC_0b00110100: { inc_reg(HL, ram); ++pc; return t_cycle_by_ins(itype);}
            case DEC_0b00xxx101: { dec_reg(read3_5(i), ram); ++pc; return t_cycle_by_ins(itype);}
            case DEC_0b00110101: { dec_reg(HL, ram); ++pc; return t_cycle_by_ins(itype);}
            case AND_0b10100xxx: { and_reg_A(read_regs(read0_2(i), ram)); ++pc; return t_cycle_by_ins(itype);}
            case AND_0b10100110: { and_reg_A(read_regs(HL, ram)); ++pc; return t_cycle_by_ins(itype);}
            case AND_0b11100110: { and_reg_A(ram.read(pc+1)); pc+=2; return t_cycle_by_ins(itype);}
            case OR_0b10110xxx:  { or_reg_A(read_regs(read0_2(i), ram)); ++pc; return t_cycle_by_ins(itype);}
            case OR_0b10110110:  { or_reg_A(read_regs(HL, ram)); ++pc; return t_cycle_by_ins(itype);}
            case OR_0b11110110:  { or_reg_A(ram.read(pc+1)); pc+=2; return t_cycle_by_ins(itype);}
            case XOR_0b10101xxx: { xor_reg_A(read_regs(read0_2(i), ram)); ++pc; return t_cycle_by_ins(itype);}
            case XOR_0b10101110: { xor_reg_A(read_regs(HL, ram)); ++pc; return t_cycle_by_ins(itype);}
            case XOR_0b11101110: { xor_reg_A(ram.read(pc+1)); pc+=2; return t_cycle_by_ins(itype);}
            case CCF_0b00111111: { set_flag(Cf, !get_flag(Cf)); set_flag(Nf, 0); set_flag(Hf, 0); ++pc; return t_cycle_by_ins(itype);}
            case SCF_0b00110111: { set_flag(Cf, 1); set_flag(Nf, 0); set_flag(Hf, 0); ++pc; return t_cycle_by_ins(itype);}
            case DAA_0b00100111: { /*todo*/ ++pc; return t_cycle_by_ins(itype);}
            case CPL_0b00101111: {regs[A] = ~regs[A]; set_flag(Nf, 1); set_flag(Hf, 1); ++pc; return t_cycle_by_ins(itype);}
            case INC_0b00xx0011: { 
                auto rr = read45(i);  
                unsigned short pair = getpair(rr); 
                ++pair;
                update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
                ++pc;
                return t_cycle_by_ins(itype);
            }
            case DEC_0b00xx1011: {
                auto rr = read45(i);  
                unsigned short pair = getpair(rr); 
                --pair;
                update_pair_ld((pair & 0xff), ((pair >> 8) & 0xff), rr);
                ++pc;
                return t_cycle_by_ins(itype);
            }
            case ADD_0b00xx1001: { add_reg_HL(getpair(read45(i))); ++pc; return t_cycle_by_ins(itype);}
            case ADD_0b11101000: {
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                auto e = static_cast<char>(ram.read(pc+1));
                int result = e + sp;
                set_flag(Cf, ((sp & 0xff) +  (unsigned char)(e) > 0xff));
                set_flag(Hf, ((sp & 0xf) + ((unsigned char)(e) & 0xf) > 0xf));
                sp = (unsigned short)(result);
                pc+=2;
                return t_cycle_by_ins(itype);
            }
            case RLCA_0b00000111: {
                auto res = std::rotl(regs[A], 1);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, regs[A] & 0x80);
                regs[A] = res;
                ++pc;  
                return t_cycle_by_ins(itype);}
            case RRCA_0b00001111: {
                auto res = std::rotr(regs[A], 1);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, regs[A] & 1);
                regs[A] = res;
                ++pc;  
                return t_cycle_by_ins(itype);}
            case RLA_0b00010111:  {
                auto carry = get_flag(Cf);
                auto res = ((regs[A] << 1) | carry);
                set_flag(Cf, regs[A] & 0x80);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                regs[A] = res;
                ++pc; return t_cycle_by_ins(itype);}
            case RRA_0b00011111:  {
                auto carry = get_flag(Cf);
                auto res = ((regs[A] >> 1) | carry << 7);
                set_flag(Cf, regs[A] & 1);
                set_flag(Zf, 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                regs[A] = res;
                ++pc;  return t_cycle_by_ins(itype);}
           
            case RLC_0xCB_0b00000xxx:  {
                auto pc_next = ram.read(pc+1);
                auto rid = read0_2(pc_next);
                auto reg = read_regs(rid, ram);
                auto res = std::rotl(reg, 1);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, reg & 0x80);
                update_regs(rid, ram, res);
                pc+=2; return t_cycle_by_ins(itype, pc_next);}
            case RRC_0xCB_0b00001xxx:  {
                auto pc_next = ram.read(pc+1);
                auto rid = read0_2(pc_next);
                auto reg = read_regs(rid, ram);
                auto res = std::rotr(reg, 1);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, reg & 1);
                update_regs(rid, ram, res);
                pc+=2; return t_cycle_by_ins(itype, pc_next);}
            case RL_0xCB_0b00010xxx:   {
                auto pc_next = ram.read(pc+1);
                auto rid = read0_2(pc_next);
                auto reg = read_regs(rid, ram);
                auto carry = get_flag(Cf);
                auto res = ((reg << 1) | carry);
                set_flag(Cf, reg & 0x80);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                update_regs(rid, ram, res);
                pc+=2; return t_cycle_by_ins(itype, pc_next);}
            case RR_0xCB_0b00011xxx:   {
                auto pc_next = ram.read(pc+1);
                auto rid = read0_2(pc_next);
                auto reg = read_regs(rid, ram);
                auto carry = get_flag(Cf);
                auto res = ((reg >> 1) | carry << 7);
                set_flag(Cf, reg & 1);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                update_regs(rid, ram, res);
                pc+=2; return t_cycle_by_ins(itype, pc_next);}
            case SLA_0xCB_0b00100xxx:  {
                auto pc_next = ram.read(pc+1);
                auto rid = read0_2(pc_next);
                auto reg = read_regs(rid, ram);
                auto res = reg << 1;
                set_flag(Cf, reg & 0x80);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                update_regs(rid, ram, res);
                pc+=2; return t_cycle_by_ins(itype, pc_next);}
            case SRA_0xCB_0b00101xxx:  {
                auto pc_next = ram.read(pc+1);
                auto rid = read0_2(pc_next);
                auto reg = read_regs(rid, ram);
                auto sign_bit = reg & 0x80;
                auto res = ((reg >> 1) | sign_bit);
                set_flag(Cf, reg & 1);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                update_regs(rid, ram, res);
                pc+=2; return t_cycle_by_ins(itype, pc_next);}
            case SWAP_0xCB_0b00110xxx: {
                auto pc_next = ram.read(pc+1);
                auto rid = read0_2(pc_next);
                auto reg = read_regs(rid, ram);
                auto l = reg & 0x0f;
                auto h = reg & 0xf0;
                auto res = (l << 4 | h >> 4);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                set_flag(Cf, 0);
                update_regs(rid, ram, res);
                pc+=2; return t_cycle_by_ins(itype, pc_next);}
            
            case SRL_0xCB_0b00111xxx:  {
                auto pc_next = ram.read(pc+1);
                auto rid = read0_2(pc_next);
                auto reg = read_regs(rid, ram);
                auto res = reg >> 1;
                set_flag(Cf, reg & 1);
                set_flag(Zf, res == 0);
                set_flag(Nf, 0);
                set_flag(Hf, 0);
                update_regs(rid, ram, res);
                pc+=2; return t_cycle_by_ins(itype, pc_next);}

            case BIT_0xCB_0b01xxxxxx:  {
                auto pc_next = ram.read(pc+1);
                auto bit_n = read3_5(pc_next);
                auto reg = read_regs(read0_2(pc_next), ram);
                set_flag(Zf, !((reg >> bit_n) & 1));
                set_flag(Nf, 0);
                set_flag(Hf, 1);
                pc+=2; return t_cycle_by_ins(itype, pc_next);}
            case RES_0xCB_0b10xxxxxx:  {
                auto pc_next = ram.read(pc+1);
                auto bit_n = read3_5(pc_next);
                auto rid = read0_2(pc_next);
                auto reg = read_regs(rid, ram);
                update_regs(rid, ram, reg & ~(1 << bit_n));
                pc+=2; return t_cycle_by_ins(itype, pc_next);}
            case SET_0xCB_0b11xxxxxx:  {
                auto pc_next = ram.read(pc+1);
                auto bit_n = read3_5(pc_next);
                auto rid = read0_2(pc_next);
                auto reg = read_regs(rid, ram);
                update_regs(rid, ram, reg | (1 << bit_n));
                pc+=2; return t_cycle_by_ins(itype, pc_next);}


            // Control flow
            case JP_0b11000011: { pc = ((ram.read(pc+2) << 8) | ram.read(pc + 1)); return t_cycle_by_ins(itype);}
            case JP_0b11101001: { pc = read_regs(HL, ram); return t_cycle_by_ins(itype);}
            case JP_0b110xx010: { 
                auto val = read34(i);
                auto jmp = [&]{pc = ((ram.read(pc+2) << 8) | ram.read(pc + 1));};
                bool cc_flag = get_cc_flag(val);
                if (cc_flag)
                    jmp();
                else 
                    pc+=3;
                return t_cycle_by_ins(itype, cc_flag);
            }
            case JR_0b00011000: {pc += 2 + (char)(ram.read(pc+1)); return t_cycle_by_ins(itype);}
            case JR_0b001xx000: {
                unsigned char val = read34(i);
                auto jmp = [&]{pc += 2 + (char)(ram.read(pc+1));};
                bool cc_flag = get_cc_flag(val);
                if (cc_flag)
                    jmp();
                else 
                    pc+=2;    
                return t_cycle_by_ins(itype, cc_flag);
            }
            case CALL_0b11001101: {
                unsigned short nn = ((ram.read(pc + 2) << 8) | ram.read(pc + 1));
                sp-=2;
                ram.write(sp, (pc + 3) & 0xff);
                ram.write(sp + 1, ((pc + 3) >> 8));
                pc = nn;
                return t_cycle_by_ins(itype);
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
                bool cc_flag = get_cc_flag(val);
                if (cc_flag)
                    call();
                else
                    pc+=3;
                return t_cycle_by_ins(itype, cc_flag);
            }
            case RET_0b11001001: { pc = ((ram.read(sp + 1) << 8) | ram.read(sp)); sp+=2; return t_cycle_by_ins(itype);}
            case RET_0b110xx000: {
                auto val = read34(i);
                auto ret = [&]{
                    pc = ((ram.read(sp + 1) << 8) | ram.read(sp)); sp+=2;
                };
                auto cc_flag = get_cc_flag(val);
                if (cc_flag)
                    ret();
                else
                    ++pc;
                return t_cycle_by_ins(itype, cc_flag);
            }
            case RETI_0b11011001: {pc = ((ram.read(sp + 1) << 8) | ram.read(sp)); sp+=2; ime = true; return t_cycle_by_ins(itype);}
            case RST_0b11xxx111: {
                unsigned short nn = ((0x0 << 8) | rst_vec[read3_5(i)]);
                sp-=2;
                ram.write(sp, (pc + 1) & 0xff);
                ram.write(sp + 1, ((pc + 1) >> 8));
                pc = nn;
                return t_cycle_by_ins(itype);
            }
            case HALT: { /*todo*/ return t_cycle_by_ins(itype);}
            case STOP: { /*todo*/ return t_cycle_by_ins(itype);}
            case DI_0b11110011: {ime = false; ++pc; return t_cycle_by_ins(itype);}
            case EI_0b11111011: {ime = true; ++pc; return t_cycle_by_ins(itype);} // todo: check
            case NOP_0b00000000: {++pc; return t_cycle_by_ins(itype);}
    }
        return 0;
    }
};


constexpr size_t DISPLAY_WIDTH = 1200;
constexpr size_t DISPLAY_HEIGHT = 600;
constexpr double CPU_FREQUENCY = 4194304.0;
constexpr double CPU_CYCLE_TIME = 1/CPU_FREQUENCY;

struct GameBoyVM{
    CPU cpu;
    RAM ram;
    PPU ppu;
    /*todo finish other components*/
    // Timer
    // Interrupts
    // Input
    // Sount

    void render(){/*todo*/}
    
    void run() {
        InitWindow(DISPLAY_WIDTH, DISPLAY_HEIGHT, "GameBoy");
        SetTargetFPS(60); // 59.7275 FPS

        // texture init
        Image img = GenImageColor(DISPLAY_WIDTH, DISPLAY_HEIGHT, BLACK);
        Texture2D target = LoadTextureFromImage(img);
        UnloadImage(img);
        std::array<std::array<Color, DISPLAY_WIDTH>, DISPLAY_HEIGHT> display_buffer;

        // init display
        for (size_t i = 0, ei = DISPLAY_HEIGHT; i < ei; ++i) {
            for (size_t j = 0, ej = DISPLAY_WIDTH; j < ej; ++j) {
                display_buffer[i][j] = BLACK;
            }
        }
        UpdateTexture(target, display_buffer.data());

        auto draw = [&]() {
             BeginDrawing();
                ClearBackground(RAYWHITE);
                // DrawFPS(1000, 10);
            EndDrawing();
        };
    
        int current_t_cycle = 0;
        double cpu_accumulator = 0.0;
        while(!WindowShouldClose()){
            cpu_accumulator += GetFrameTime();
            while(current_t_cycle < 70'224 && cpu_accumulator >= CPU_CYCLE_TIME) {
                auto next_ins = cpu.fetch(ram); 
                auto decoded_ins = cpu.decode(next_ins);
                auto t_cycle = cpu.execute(next_ins, decoded_ins, ram);        
                ppu.handle_t_cycle(t_cycle);
                current_t_cycle+=t_cycle;
                cpu_accumulator-=CPU_CYCLE_TIME;
            }

            if (ppu.get_current_mode() == PPU::VBlank) render();
            current_t_cycle = 0;

            draw();
        }
    }
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
            case 1: return LIGHTGRAY;
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

int get_rom_size(const auto& data) {
    switch(data[0x0148]) {
        case 0x1: return 4;   // 64KB
        case 0x2: return 8;   // 128KB
        case 0x3: return 16;  // 256KB
        case 0x4: return 32;  // 512KB
        case 0x5: return 64;  // 1MB
        case 0x6: return 128; // 2MB
        case 0x7: return 256; // 4MB
        case 0x8: return 512; // 8MB
        case 0x52: return 72; // 1.1MB
        case 0x53: return 80; // 1.2MB
        case 0x54: return 96; // 1.5MB
    }
    return 2; // 32KB
}

int get_ram_size(const auto& data) {
    switch(data[0x0149]) {
        case 0x2: return 1; // 1 bank (8KiB)
        case 0x3: return 4; // 4 banks of 8KiB (32KiB)
        case 0x4: return 16; // 16 banks of 8KiB (128KiB)
        case 0x5: return 8; // 8 banks of 8KiB (64KiB)
    }
    // 0x01 - Unused
    // Listed in various unofficial docs as 2 KiB. However, a 2 KiB RAM chip was never used in a cartridge. 
    // The source of this value is unknown.
    return 0; // No ram
}

int main() {
    // test tiles draw
    unsigned char data[16]{0xFF, 0x00, 0x7E, 0xFF, 0x85, 0x81, 0x89, 0x83, 0x93, 0x85, 0xA5, 0x8B, 0xC9, 0x97, 0x7E, 0xFF};
    draw_tiles(data);
}
