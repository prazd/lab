#include <iostream>
#include <array>
#include "raylib.h"
#include <fstream>
#include <filesystem>


#define DEBUG 0

struct Chip8VM {
    enum GRegisters {V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, VA, VB, VC, VD, VE, VF};
    constexpr static unsigned char FONT_OFFSET = 0x50;

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
        Invalid
    };

#ifdef DEBUG
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
    unsigned short index{};                   // index register
    unsigned short pc{};                      // program counter
    unsigned char  delay_timer{};             // An 8-bit delay timer which is decremented at a rate of 60 Hz (60 times per second) until it reaches 0
    unsigned char  sound_timer{};             // An 8-bit sound timer which functions like the delay timer, but which also gives off a beeping sound as long as it’s not 0
    unsigned char  sp{};                      // stack pointer
    
    std::array<unsigned short, 16> stack{}; // stack is an array of 16 16-bit values
                                            // used to store the address that the interpreter should return to when finished with a subroutine
    
    std::array<unsigned char, 16>  regs{};  // 16 general purpose 8-bit registers
    std::array<unsigned char, 4096> mem{};  // 4kb RAM

    // 64 x 32
    std::array<std::array<bool, 128>, 32> display;

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

    const auto& cdisplay() const {
        return display;
    }

    std::array<unsigned char, 16> keyboard = {
         1,  2,  3,  0xC,
         4,  5,  6,  0xD,
         7,  8,  9,  0xE,
        0xA, 0, 0xB, 0xF,
    };

    std::array<unsigned char, 16> raylib_keyboard = {
        KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR,
        KEY_Q, KEY_W, KEY_E, KEY_R,
        KEY_A, KEY_S, KEY_D, KEY_F, 
        KEY_Z, KEY_X, KEY_C, KEY_V,
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

    void load_instructions() {
        // load fonts
        std::copy(fonts.begin(), fonts.end(), mem.data() + FONT_OFFSET);
    }

    void key_handler(unsigned short i) {
        for (size_t i = 0; i < 16; ++i) {
            if (IsKeyPressed(raylib_keyboard[i])) {
                regs[readx(i)] = keyboard[i];
                key_pressed_list[i] = true;
                return;
            }
        }
    }

    Instruction decode (unsigned short i) {
        switch(i >> 12) {
            case 0: {
                switch(i & 0xff) {
                    case 0xE0: {return CLS_00E0;}
                    case 0xEE: {return RET_00EE;}
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
        load_instructions();
        clear_screen();
        pc = 0x200;
        std::memcpy(mem.data() + pc, instructions.data(), std::size(instructions) * sizeof(typename Ctr::value_type));
        while((mem[pc + 1] | mem[pc] << 8)) {
            dec_exec((mem[pc + 1] | mem[pc] << 8)); 
            pc += 2;
        }
    }

    void dec_exec(unsigned short i) {
        auto decoded = decode(i);
        #ifdef DEBUG 
        std::cout << std::hex << "Instruction: " << instruction_str[decoded] << " 0x"<< i << '\n';
        std::cout << std::dec;
        #endif
        switch(decoded) {
            case SYS_0nnn: { /* check */ break;}
            case CLS_00E0: { clear_screen(); break;};
            case RET_00EE: { pc = stack[sp--]; break;}
            case JP_1nnn:  { pc = (0xFFF & i);  
                std::cout << "JP_1nnn PC:" << pc << '\n';
                break;} 
            case CALL_2nnn:{ ++sp; stack[sp] = pc; pc = 0xFFF & i; break;}
            case SE_3xkk:  { pc += regs[readx(i)] == (i & 0xFF) ? 2 : 0; break;}
            case SNE_4xkk: { pc += regs[readx(i)] == (i & 0xFF) ? 0 : 2; break;}
            case SE_5xy0:  { pc += regs[readx(i)] == regs[ready(i)] ? 2 : 0; break;}
            case LD_6xkk:  {regs[readx(i)] = (i & 0xFF);     break;}
            case ADD_7xkk: {regs[readx(i)] += (i & 0xFF);     break;}
            case LD_8xy0:  {regs[readx(i)] = regs[ready(i)];  break;}
            case OR_8xy1:  {regs[readx(i)] |= regs[ready(i)]; break;}
            case AND_8xy2: {regs[readx(i)] &= regs[ready(i)]; break;}
            case XOR_8xy3: {regs[readx(i)] ^= regs[ready(i)]; break;}
            case ADD_8xy4: {int val = regs[readx(i)] + regs[ready(i)]; regs[VF] = val > 255; regs[readx(i)] = val & 0xff; break;}
            case SUB_8xy5: {regs[VF] = regs[readx(i)] > regs[ready(i)]; regs[readx(i)] -= regs[ready(i)]; break;}
            case SHR_8xy6: {regs[VF] = (regs[readx(i)] & 1); regs[readx(i)]>>=1; break;}
            case SUBN_8xy7:{regs[VF] = regs[ready(i)] > regs[readx(i)]; regs[readx(i)] = regs[ready(i)] - regs[readx(i)]; break;}
            case SHL_8xyE: {regs[VF] = (regs[readx(i)] & 0x80); regs[readx(i)] <<= 1; break;}
            case SNE_9xy0: {pc += regs[readx(i)] != regs[ready(i)] ? 2 : 0; break;}
            case LD_Annn:  {
                index = (0xFFF & i); 
                std::cout << "\nLD_Annn index:" << index << '\n';
                break;}
            case JP_Bnnn:  {pc = (0xFFF & i) + regs[V0]; break;}
            case RND_Cxkk: { regs[readx(i)] = ((rand() % 256) & (0xFF & i)); break;} // todo: update rand
            case DRW_Dxyn: { 
                auto x = regs[readx(i)];
                auto y = regs[ready(i)];
                auto nibble = i & 0xF;
                regs[VF] = 0;
                for (size_t j = 0; j < nibble; ++j){
                    unsigned char current = mem[index + j];      // X = 10, y = 20; n = 2; mem[index] = 0b01001111, mem[index + 1] = 0b10011000
                    auto cx = x;                                 // y[20]: ... x[10] = 0, x[11] = 1 ... x[17] = 1
                    for (size_t k = 0; k < CHAR_BIT; ++k) {      // y[21]: ... x[10] = 1, x[11] = 0 ... x[17] = 0
                        auto before = display[y + j][cx];
                        display[y + j][cx] ^= ((current >> (7 - k)) & 1);
                        if (before && !display[y + j][cx]) regs[VF] = 1;
                        ++cx;
                    }
                }
                break;} // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
            case SKP_Ex9E:  {pc += (key_pressed_list[regs[readx(i)]]) ? 2 : 0; break;} // Skip next instruction if key with the value of Vx is pressed.
            case SKNP_ExA1: {pc += (key_pressed_list[regs[readx(i)]]) ? 0 : 2; break;} // Skip next instruction if key with the value of Vx is not pressed.
            case LD_Fx07: { regs[readx(i)] = delay_timer; break;}
            case LD_Fx0A: { 
                key_handler(i);
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
                auto current = index;
                for (int k = 0; k <= readx(i); ++k, ++current)
                    mem[current] = regs[k];
                break;} // Store registers V0 through Vx in memory starting at location I
            case LD_Fx65: {
                auto current = index;
                for (int k = 0; k <= readx(i); ++k, ++current)
                    regs[k] = mem[current];
                break;} // Read registers V0 through Vx from memory starting at location I.    

            case Invalid:{
                std::cout << std::hex << "!!!! Invalid instruction: " << i << '\n';
                std::cout << std::dec;
                break;
            }    
        }
    }
};

auto rom_to_raw_instructions(const std::string_view& path) {
    std::ifstream f{path, std::ios::binary | std::ios::in };
    if (!f.is_open()) {std::cout << "Cannot open file"; std::abort();}
    auto sz = std::filesystem::file_size(path);
    std::vector<char> result(sz);
    f.seekg(0, std::ios::beg);
    f.read(result.data(), sz);
    f.close();
    return result;
}

int main() {
    Chip8VM vm;
    vm.run(rom_to_raw_instructions("/Users/prazd/Desktop/books/cpp/cpp/vm/chip8/test-roms/test_opcode.ch8"));
    for (auto&& row: vm.display){
        for (auto& el: row)
            std::cout << (el ? "0" : ".");
        std::cout << "\n";
    }
}
