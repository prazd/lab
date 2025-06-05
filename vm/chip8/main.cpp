#include <iostream>
#include <array>
#include "raylib.h"
#include <fstream>
#include <filesystem>
#include <random>

#define DEBUG 1

constexpr bool LEGACY_SHIFT = true;

constexpr float HEIGHT = 600.f;
constexpr float WIDTH = 1200.f;

constexpr size_t CLASSIC_HEIGHT = 32;
constexpr size_t CLASSIC_WIDTH = 64;

constexpr size_t SCHIP_HEIGHT = 64;
constexpr size_t SCHIP_WIDTH = 128;

constexpr size_t DISPLAY_HEIGHT = 64;
constexpr size_t DISPLAY_WIDTH = 128;

template <std::integral T, T begin, T end>
class RandomInt {
    static inline std::random_device rd;
    static inline std::mt19937 gen{rd()};
    static inline std::uniform_int_distribution<int> dis = std::uniform_int_distribution<T>(begin, end);
public:
    static auto rand() {
        return dis(gen);
    }
};

struct Chip8VM {
    enum GRegisters {V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VA, VB, VC, VD, VE, VF};

    enum Mode {Classic, SCHIP};
    
    Mode current_mode = Classic;

    constexpr static unsigned char FONT_OFFSET = 0x50;
    constexpr static unsigned short HR_FONT_OFFSET = 0x100;

    // http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#00E0
    
    // nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
    // n or nibble - A 4-bit value, the lowest 4 bits of the instruction
    // x - A 4-bit value, the lower 4 bits of the high byte of the instruction
    // y - A 4-bit value, the upper 4 bits of the low byte of the instruction
    // kk or byte - An 8-bit value, the lowest 8 bits of the instruction
    enum Instruction{
        SYS_0nnn,
        CLS_00E0,
        RET_00EE,
        JP_1nnn, 
        CALL_2nnn, 
        SE_3xkk, 
        SNE_4xkk, 
        SE_5xy0,
        LD_6xkk, 
        ADD_7xkk,
        LD_8xy0,
        OR_8xy1, 
        AND_8xy2, 
        XOR_8xy3,
        ADD_8xy4,
        SUB_8xy5, 
        SHR_8xy6,
        SUBN_8xy7,
        SHL_8xyE, 
        SNE_9xy0,
        LD_Annn,
        JP_Bnnn,
        RND_Cxkk, 
        DRW_Dxyn,
        SKP_Ex9E,
        SKNP_ExA1,
        LD_Fx07,
        LD_Fx0A,
        LD_Fx15,
        LD_Fx18,
        LD_Fx1E,
        LD_Fx29,
        LD_Fx33,
        LD_Fx55,
        LD_Fx65,
        
        // super chip
        SCD_00CN, 
        SCR_00FB,
        SCL_00FC,
        EXIT_00FD,
        LOW_00FE,
        HIGH_00FF,
        LD_Fx30,
        LD_Fx75,
        LD_Fx85,

        Invalid
    };

#if DEBUG
    std::unordered_map<Instruction, std::string_view> instruction_str {
        {SYS_0nnn,  "SYS_0nnn" },
        {CLS_00E0,  "CLS_00E0" },
        {RET_00EE,  "RET_00EE" },
        {JP_1nnn,   "JP_1nnn"  },
        {CALL_2nnn, "CALL_2nnn"},
        {SE_3xkk,   "SE_3xkk"  },
        {SNE_4xkk,  "SNE_4xkk" },
        {SE_5xy0,   "SE_5xy0"  },
        {LD_6xkk,   "LD_6xkk"  },
        {ADD_7xkk,  "ADD_7xkk" },
        {LD_8xy0,   "LD_8xy0"  },
        {OR_8xy1,   "OR_8xy1"  },
        {AND_8xy2,  "AND_8xy2" },  
        {XOR_8xy3,  "XOR_8xy3" },  
        {ADD_8xy4,  "ADD_8xy4" }, 
        {SUB_8xy5,  "SUB_8xy5" },  
        {SHR_8xy6,  "SHR_8xy6" },
        {SUBN_8xy7, "SUBN_8xy7"},      
        {SHL_8xyE,  "SHL_8xyE" },   
        {SNE_9xy0,  "SNE_9xy0" },
        {LD_Annn,   "LD_Annn"  },
        {JP_Bnnn,   "JP_Bnnn"  },
        {RND_Cxkk,  "RND_Cxkk" },
        {DRW_Dxyn,  "DRW_Dxyn" },
        {SKP_Ex9E,  "SKP_Ex9E" },
        {SKNP_ExA1, "SKNP_ExA1"},
        {LD_Fx07,   "LD_Fx07"  },
        {LD_Fx0A,   "LD_Fx0A"  },
        {LD_Fx15,   "LD_Fx15"  },
        {LD_Fx18,   "LD_Fx18"  },
        {LD_Fx1E,   "LD_Fx1E"  },
        {LD_Fx29,   "LD_Fx29"  },
        {LD_Fx33,   "LD_Fx33"  },
        {LD_Fx55,   "LD_Fx55"  },
        {LD_Fx65,   "LD_Fx65"  },
        {Invalid,   "Invalid"  },
    }; 
#endif
    bool press_wait = false;
    unsigned short index{};                   // index register
    unsigned short pc{};                      // program counter
    unsigned char  delay_timer{};             // An 8-bit delay timer which is decremented at a rate of 60 Hz (60 times per second) until it reaches 0
    unsigned char  sound_timer{};             // An 8-bit sound timer which functions like the delay timer, but which also gives off a beeping sound as long as it’s not 0
    unsigned char  sp{};                      // stack pointer
    
    std::array<unsigned short, 16> stack{}; // stack is an array of 16 16-bit values
                                            // used to store the address that the interpreter should return to when finished with a subroutine
    
    std::array<unsigned char, 16>  regs{};  // 16 general purpose 8-bit registers
    std::array<unsigned char, 4096> mem{};  // 4kb RAM


    std::array<unsigned char, 8> rpl{};     // super chip

    // 64 x 32
    std::array<std::array<bool, DISPLAY_WIDTH>, DISPLAY_HEIGHT> display{};

    std::array<unsigned char, 80> fonts{
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    // super chip
    std::array<unsigned char, 160> high_res_font {
        0xF0,0xF0,0x90,0x90,0x90,0x90,0x90,0x90,0xF0,0xF0, // 0
        0x20,0x20,0x60,0x60,0x20,0x20,0x20,0x20,0x70,0x70, // 1
        0xF0,0xF0,0x10,0x10,0xF0,0xF0,0x80,0x80,0xF0,0xF0, // 2
        0xF0,0xF0,0x10,0x10,0xF0,0xF0,0x10,0x10,0xF0,0xF0, // 3
        0x90,0x90,0x90,0x90,0xF0,0xF0,0x10,0x10,0x10,0x10, // 4
        0xF0,0xF0,0x80,0x80,0xF0,0xF0,0x10,0x10,0xF0,0xF0, // 5
        0xF0,0xF0,0x80,0x80,0xF0,0xF0,0x90,0x90,0xF0,0xF0, // 6
        0xF0,0xF0,0x10,0x10,0x20,0x20,0x40,0x40,0x40,0x40, // 7
        0xF0,0xF0,0x90,0x90,0xF0,0xF0,0x90,0x90,0xF0,0xF0, // 8
        0xF0,0xF0,0x90,0x90,0xF0,0xF0,0x10,0x10,0xF0,0xF0, // 9
        0xF0,0xF0,0x90,0x90,0xF0,0xF0,0x90,0x90,0x90,0x90, // A
        0xE0,0xE0,0x90,0x90,0xE0,0xE0,0x90,0x90,0xE0,0xE0, // B
        0xF0,0xF0,0x80,0x80,0x80,0x80,0x80,0x80,0xF0,0xF0, // C
        0xE0,0xE0,0x90,0x90,0x90,0x90,0x90,0x90,0xE0,0xE0, // D
        0xF0,0xF0,0x80,0x80,0xF0,0xF0,0x80,0x80,0xF0,0xF0, // E
        0xF0,0xF0,0x80,0x80,0xF0,0xF0,0x80,0x80,0x80,0x80  // F
    };

    const auto& cdisplay() const {
        return display;
    }

    std::array<unsigned char, 16> keyboard = {
        0,  1,  2,  3,
        4,  5,  6,  7,
        8,  9,  0xa, 0xb,
        0xc, 0xd, 0xe, 0xf,
    };

    std::array<unsigned char, 16> raylib_keyboard = {
        KEY_X, KEY_ONE, KEY_TWO, KEY_THREE,
        KEY_Q, KEY_W, KEY_E, KEY_A,
        KEY_S, KEY_D, KEY_Z, KEY_C,
        KEY_FOUR, KEY_R, KEY_F, KEY_V,
    };

    std::array<bool, 16> key_pressed_list;

    // x - A 4-bit value, the lower 4 bits of the high byte of the instruction
    unsigned short readx(unsigned short i) {
        return 0xF & (i >> 8);
    }

    // y - A 4-bit value, the upper 4 bits of the low byte of the instruction
    unsigned short ready(unsigned short i) {
        return 0xF & (i >> 4);
    }

    template <typename Ctr>
    void load_instructions(const Ctr& instructions) {
        pc = 0x200;
        clear_screen();
        std::memcpy(mem.data() + pc, instructions.data(), std::size(instructions) * sizeof(typename Ctr::value_type));
        std::copy(fonts.begin(), fonts.end(), mem.data() + FONT_OFFSET);
        std::copy(high_res_font.begin(), high_res_font.end(), mem.data() + HR_FONT_OFFSET);
    }

    void key_handle() {
        for (size_t i = 0; i < 16; ++i){
            key_pressed_list[i] = IsKeyDown(raylib_keyboard[i]);
        }
    }

    void wait_for_press(unsigned short i) {
        for (size_t k = 0, e = std::size(key_pressed_list); k < e; ++k) {
            if (key_pressed_list[k]) {
                regs[readx(i)] = k;
                press_wait = false;
                return;
            }
        }
    }

    Instruction decode (unsigned short i) {
        switch(i >> 12) {
            case 0: {
                if (((0xf0 & i) >> 1) == 0xC) {
                    return SCD_00CN;
                }
                switch(i & 0xff) {
                    case 0xE0: {return CLS_00E0;}
                    case 0xEE: {return RET_00EE;}
                    // super chip
                    case 0xFB: {return SCR_00FB;}
                    case 0xFC: {return SCL_00FC;}
                    case 0xFD: {return EXIT_00FD;}
                    case 0xFE: {return LOW_00FE;}
                    case 0xFF: {return HIGH_00FF;}
                    default:   {return SYS_0nnn;}
                }
            }
            case 1: { return JP_1nnn; }
            case 2: { return CALL_2nnn;}
            case 3: { return SE_3xkk;}
            case 4: { return SNE_4xkk;}
            case 5: { return SE_5xy0;}
            case 6: { return LD_6xkk;}
            case 7: { return ADD_7xkk;}
            case 8: {
                switch(0xf & i){
                    case 0:   {return LD_8xy0;}
                    case 1:   {return OR_8xy1;}
                    case 2:   {return AND_8xy2;}
                    case 3:   {return XOR_8xy3;}
                    case 4:   {return ADD_8xy4;}
                    case 5:   {return SUB_8xy5;}
                    case 6:   {return SHR_8xy6;}
                    case 7:   {return SUBN_8xy7;}
                    case 0xE: {return SHL_8xyE;}
                }
            }
            case 9: {return SNE_9xy0;}
            case 0xA: {return LD_Annn;}
            case 0xB: {return JP_Bnnn;}
            case 0xC: {return RND_Cxkk;}
            case 0xD: {return DRW_Dxyn;}
            case 0xE: {
                switch(i & 0xff) {
                    case 0x9E: {return SKP_Ex9E;}
                    case 0xA1: {return SKNP_ExA1;}
                }
            }
            case 0xF: {
                switch(i & 0xff) {
                    case 0x07:  {return LD_Fx07;}
                    case 0x0A:  {return LD_Fx0A;}
                    case 0x15:  {return LD_Fx15;}
                    case 0x18:  {return LD_Fx18;}
                    case 0x1E:  {return LD_Fx1E;}
                    case 0x29:  {return LD_Fx29;}
                    case 0x33:  {return LD_Fx33;}
                    case 0x55:  {return LD_Fx55;}
                    case 0x65:  {return LD_Fx65;}
                    // super chip
                    case 0x30:  {return LD_Fx30;}
                    case 0x75:  {return LD_Fx75;}
                    case 0x85:  {return LD_Fx85;}
                }
            }
        }
        return Invalid;
    }

    void clear_screen(){
        for (auto&& row: display)
            row.fill(false);
    }

    template <typename Ctr>
    void run(const Ctr& instructions) {
        load_instructions(instructions);
        while((mem[pc + 1] | mem[pc] << 8)) {
            auto current_pc = std::exchange(pc, pc + 2);
            dec_exec((mem[current_pc + 1] | mem[current_pc] << 8)); 
        }
    }

    void step() {
        if (sound_timer) { std::cout << "BEEP!\n"; --sound_timer;}
        if (delay_timer) --delay_timer;
        if ((mem[pc + 1] | mem[pc] << 8)) {
            auto current_pc = std::exchange(pc, pc + 2);
            dec_exec((mem[current_pc + 1] | mem[current_pc] << 8));
            if (press_wait) pc = current_pc;
        }
    }

    size_t width() const {
        return current_mode == Classic ? CLASSIC_WIDTH : SCHIP_WIDTH;
    }

    size_t height() const {
        return current_mode == Classic ? CLASSIC_HEIGHT : SCHIP_HEIGHT;
    }

    void scroll_screen_down(size_t n) {
        std::shift_right(display.begin(), display.end(), n);
        for (size_t i = 0; i < n; ++i) display[i].fill(false);
    }

    void scroll_screen_right(size_t n) {
        for (auto&& row: display){
            std::shift_right(row.begin(), row.end(), n);
            for (size_t i = 0; i < n; ++i) row[i] = false;
        }
    }

    void scroll_screen_left(size_t n) {
        for (auto&& row: display){
            std::shift_left(row.begin(), row.end(), n);
            for (size_t i = width() - n; i < width(); ++i) row[i] = false;
        }
    }

    void draw16x16(size_t x, size_t y) {
        for (size_t j = 0; j < 32; j+=2){
            unsigned short current = (mem[index + j*2] << 8| mem[index + j*2 + 1]);
            auto cx = x;                           
            for (size_t k = 0; k < 16; ++k) {      
                auto before = display[(y + j) % height()][cx % width()];
                display[(y + j) % height()][cx % width()] ^= ((current >> (15 - k)) & 1);
                if (before && !display[(y + j) % height()][cx % width()]) regs[VF] = 1;
                ++cx;
            }
        }
    }

    void dec_exec(unsigned short i) {
        auto decoded = decode(i);
        
        #if DEBUG 
        std::cout << std::hex << " Instruction: " << instruction_str[decoded] << " 0x" << i;
        std::cout << std::dec << " PC:" << pc << " I: " << index << '\n';
        #endif  

        switch(decoded) {
            case SYS_0nnn: {/* ignored */      break;}
            case CLS_00E0: { clear_screen();   break;}
            case RET_00EE: { pc = stack[sp--]; break;}
            case JP_1nnn:  { pc = (0xFFF & i); break;} 
            case CALL_2nnn:{stack[++sp] = std::exchange(pc, 0xfff & i); break;}
            case SE_3xkk:  { pc += regs[readx(i)] == (i & 0xff) ? 2 : 0; break;}
            case SNE_4xkk: { pc += regs[readx(i)] == (i & 0xff) ? 0 : 2; break;}
            case SE_5xy0:  { pc += regs[readx(i)] == regs[ready(i)] ? 2 : 0; break;}
            case LD_6xkk:  {regs[readx(i)]  = (i & 0xff);       break;}
            case ADD_7xkk: {regs[readx(i)] += (i & 0xff);      break;}
            case LD_8xy0:  {regs[readx(i)]  = regs[ready(i)];  break;}
            case OR_8xy1:  {regs[readx(i)] |= regs[ready(i)];  break;}
            case AND_8xy2: {regs[readx(i)] &= regs[ready(i)];  break;}
            case XOR_8xy3: {regs[readx(i)] ^= regs[ready(i)];  break;}
            case ADD_8xy4: {int val = regs[readx(i)] + regs[ready(i)]; auto vf_after = val > 0xff; regs[readx(i)] = val & 0xff; regs[VF] = vf_after; break;}
            case SUB_8xy5: {auto vf_after = (regs[readx(i)] >= regs[ready(i)]); regs[readx(i)] -= regs[ready(i)]; regs[VF] = vf_after; break;}
            case SHR_8xy6: {
                auto& x = regs[readx(i)];
                auto y = regs[ready(i)];
                unsigned char vf_after{};
                if constexpr (LEGACY_SHIFT) {
                    vf_after = (y & 1);
                    x = (y >> 1);
                } else {
                    vf_after = (x & 1);
                    x >>= 1;
                }
                regs[VF] = vf_after;
                break;}
            case SUBN_8xy7:{auto vf_after = (regs[ready(i)] >= regs[readx(i)]); regs[readx(i)] = ((regs[ready(i)] - regs[readx(i)]) & 0xff); regs[VF] = vf_after; break;}
            case SHL_8xyE: {
                auto& x = regs[readx(i)];
                auto y = regs[ready(i)];
                unsigned char vf_after{};
                if constexpr (LEGACY_SHIFT) {
                    vf_after = ((y & 0x80) >> 7);
                    x = (y << 1);
                } else {
                    vf_after = ((x & 0x80) >> 7);
                    x <<= 1;
                }
                regs[VF] = vf_after;
                break;}
            case SNE_9xy0: {pc += regs[readx(i)] != regs[ready(i)] ? 2 : 0; break;}
            case LD_Annn:  {index = (0xFFF & i);break;}
            case JP_Bnnn:  {pc = (0xFFF & i) + regs[V0]; break;}
            case RND_Cxkk: { regs[readx(i)] = ((RandomInt<int, 0,255>::rand()) & (0xFF & i)); break;}
            case DRW_Dxyn: {
                auto x = regs[readx(i)];
                auto y = regs[ready(i)];
                auto nibble = i & 0xF;
                regs[VF] = 0;
                if (current_mode == SCHIP && !nibble) {
                    draw16x16(x, y);
                    break;
                }
                for (size_t j = 0; j < nibble; ++j){
                    unsigned char current = mem[index + j];      // X = 10, y = 20; n = 2; mem[index] = 0b01001111, mem[index + 1] = 0b10011000
                    auto cx = x;                                 // y[20]: ... x[10] = 0, x[11] = 1 ... x[17] = 1
                    for (size_t k = 0; k < CHAR_BIT; ++k) {      // y[21]: ... x[10] = 1, x[11] = 0 ... x[17] = 0
                        auto before = display[(y + j) % height()][cx % width()];
                        display[(y + j) % height()][cx % width()] ^= ((current >> (7 - k)) & 1);
                        if (before && !display[(y + j) % height()][cx % width()]) regs[VF] = 1;
                        ++cx;
                    }
                }
                break;} // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
            case SKP_Ex9E:  {pc += (key_pressed_list[regs[readx(i)]]) ? 2 : 0; break;} // Skip next instruction if key with the value of Vx is pressed.
            case SKNP_ExA1: {pc += (key_pressed_list[regs[readx(i)]]) ? 0 : 2; break;} // Skip next instruction if key with the value of Vx is not pressed.
            case LD_Fx07: { regs[readx(i)] = delay_timer; break;}
            case LD_Fx0A: { 
                press_wait = true;
                wait_for_press(i);
                break;
            } // Wait for a key press, store the value of the key in Vx.
            case LD_Fx15: {delay_timer = regs[readx(i)]; break;}
            case LD_Fx18: {sound_timer = regs[readx(i)]; break;}
            case LD_Fx1E: {index += regs[readx(i)]; break;}
            case LD_Fx29: {index = (regs[readx(i)] * 5) + FONT_OFFSET; break;} // Set I = location of sprite for digit Vx (0 - 0, 1 - 5 ...)
            case LD_Fx33: {
                auto val = regs[readx(i)];         //  val = 123
                mem[index] = val / 100;            //  1
                mem[index + 1] = (val % 100) / 10; //  2
                mem[index + 2] = val % 10;         //  3
                break;} // Store BCD representation of Vx in memory locations I, I+1, and I+2.
            case LD_Fx55: {
                for (int k = 0; k <= readx(i); ++k, ++index)
                    mem[index] = regs[k];
                break;} // Store registers V0 through Vx in memory starting at location I
            case LD_Fx65: {
                for (int k = 0; k <= readx(i); ++k, ++index)
                    regs[k] = mem[index];
                break;} // Read registers V0 through Vx from memory starting at location I.    


            // super chip
            case LD_Fx30: {index = (regs[readx(i)] * 10) + HR_FONT_OFFSET; break;}
            case SCD_00CN: { scroll_screen_down(i & 0xf); break;}
            case SCR_00FB: { scroll_screen_right(4); break;}
            case SCL_00FC: { scroll_screen_left(4);  break;}
            case EXIT_00FD: { break;} // todo
            case LOW_00FE:  {current_mode = Classic; break;}
            case HIGH_00FF: {current_mode = SCHIP; break;}
            case LD_Fx75: {
                for (int k = 0; k <= readx(i); ++k)
                    rpl[k] = regs[k];
                break;
            };
            case LD_Fx85: {
                for (int k = 0; k <= readx(i); ++k)
                    regs[k] = rpl[k];
                break;
            }
            case Invalid:{
                std::cout << std::hex << "!!!! Invalid instruction: " << i << '\n';
                std::cout << std::dec;
                break;
            }    
        }
    }
};

auto read_rom(const std::string_view& path) {
    std::ifstream f{path, std::ios::binary | std::ios::in };
    if (!f.is_open()) {std::cout << "Cannot open file"; std::abort();}
    auto sz = std::filesystem::file_size(path);
    std::vector<char> result(sz);
    f.seekg(0, std::ios::beg);
    f.read(result.data(), sz);
    f.close();
    return result;
}

template <size_t display_width, size_t display_height>
auto create_rects() {
    std::array<std::array<Rectangle, display_width>, display_height> result{};
    float x = 0.0f;
    float y = 0.0f;
    
    auto rec_width  = WIDTH/display_width;
    auto rec_height = HEIGHT/display_height;

    for (auto&& row: result) {
        for (auto& el: row) {
            el = Rectangle{x,y,rec_width, rec_height};
            x += rec_width;
        }
        x = 0.0f;
        y += rec_height;
    }
    return result;
}

int main() {
    InitWindow(WIDTH, HEIGHT, "CHIP-8");
    SetTargetFPS(300);
    auto target = LoadRenderTexture(WIDTH, HEIGHT);

    Chip8VM vm;
    vm.load_instructions(read_rom("/Users/prazd/Desktop/books/cpp/cpp/vm/chip8/test-roms/4-flags.ch8"));

    auto rects_64x32 = create_rects<CLASSIC_WIDTH, CLASSIC_HEIGHT>();
    auto rects_128x64 = create_rects<SCHIP_WIDTH, SCHIP_HEIGHT>();

    auto draw = [&](auto& arr) {
        for (size_t i = 0, ei = vm.height(); i < ei; ++i) {
            for (size_t j = 0, ej = vm.width(); j < ej; ++j) {
                DrawRectangleRec(arr[i][j], vm.display[i][j] ? GREEN : BLACK);
            }
       }
    };

    auto render = [&]{
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);
            vm.key_handle();
            vm.step();
            vm.current_mode == Chip8VM::Mode::Classic ? draw(rects_64x32) : draw(rects_128x64);
        EndTextureMode();
    };

    while(!WindowShouldClose()){
        render();
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, -(float)target.texture.height}, {0, 0}, WHITE);
            // DrawFPS(1000, 10);
        EndDrawing();
    }
}
