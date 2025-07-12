#ifndef GAMEBOY_CPU_H
#define GAMEBOY_CPU_H
#include "Constants.h"
#include "RAM.h"

struct CPU {
    static constexpr unsigned char HL = 0b110;
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
        CP_0b10111110,
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

        _0xCB,


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

    enum Instruction0xCB{
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
        NOP_0xCB_0b00000000
    };



    int t_cycle_by_ins(Instruction ins, unsigned char value = 0);
    int t_cycle_by_ins_0xCB(Instruction0xCB ins, unsigned char value = 0);

    void set_flag(Flag f, bool val);
    unsigned char get_flag(Flag f) const;

    void decrement_pair(Register l, Register h);
    void increment_pair(Register l, Register h);

    void update_pair_ld(unsigned char l,unsigned char h, unsigned char pair);
    void update_pair_pop(unsigned char l, unsigned char h, unsigned char pair);
    unsigned short getpair(unsigned char i);

    unsigned char read45(unsigned char i);
    unsigned char to_pair45(unsigned char i);
    unsigned char read34(unsigned char i);
    unsigned char read345(unsigned char i);
    unsigned char read012(unsigned char i);

    void add_reg_A(unsigned char val);
    void add_reg_HL(unsigned short val);
    void sub_reg_A(unsigned char val);
    void and_reg_A(unsigned char val);
    void or_reg_A(unsigned char val);
    void xor_reg_A(unsigned char val);
    void sub_reg_A_flags_only(unsigned char val);

    void inc_reg(unsigned char i, RAM& ram);
    void dec_reg(unsigned char i, RAM& ram);

    unsigned short HL_data() const;

    unsigned char read_regs(unsigned char i, RAM& ram);
    void update_regs(unsigned char i, RAM& ram, unsigned char value);

    bool get_cc_flag(unsigned char val);

    unsigned char fetch(const RAM& ram) const;
    Instruction decode(unsigned char ins);
    Instruction0xCB decode_0xCB(unsigned char ins);
    int execute(unsigned char i, Instruction itype, RAM& ram);
    void print_regs() const;

    std::unordered_map<Instruction, std::string_view> instruction_str = {
            {LD_0b01xxxyyy,"LD_0b01xxxyyy"},
            {LD_0b00xxx110,"LD_0b00xxx110"},
            {LD_0b01xxx110,"LD_0b01xxx110"},
            {LD_0b01110xxx,"LD_0b01110xxx"},
            {LD_0b00110110,"LD_0b00110110"},
            {LD_0b00001010,"LD_0b00001010"},
            {LD_0b00011010,"LD_0b00011010"},
            {LD_0b00000010,"LD_0b00000010"},
            {LD_0b00010010,"LD_0b00010010"},
            {LD_0b11111010,"LD_0b11111010"},
            {LD_0b11101010,"LD_0b11101010"},
            {LDH_0b11110010,"LDH_0b11110010"},
            {LDH_0b11100010,"LDH_0b11100010"},
            {LDH_0b11110000,"LDH_0b11110000"},
            {LDH_0b11100000,"LDH_0b11100000"},
            {LD_0b00111010,"LD_0b00111010"},
            {LD_0b00110010,"LD_0b00110010"},
            {LD_0b00101010,"LD_0b00101010"},
            {LD_0b00100010,"LD_0b00100010"},
            {LD_0b00xx0001,"LD_0b00xx0001"},
            {LD_0b00001000,"LD_0b00001000"},
            {LD_0b11111001,"LD_0b11111001"},
            {PUSH_0b11xx0101,"PUSH_0b11xx0101"},
            {POP_0b11xx0001,"POP_0b11xx0001"},
            {LD_0b11111000,"LD_0b11111000"},
            {ADD_0b10000xxx,"ADD_0b10000xxx"},
            {ADD_0b10000110,"ADD_0b10000110"},
            {ADD_0b11000110,"ADD_0b11000110"},
            {ADC_0b10001xxx,"ADC_0b10001xxx"},
            {ADC_0b10001110,"ADC_0b10001110"},
            {ADC_0b11001110,"ADC_0b11001110"},
            {SUB_0b10010xxx,"SUB_0b10010xxx"},
            {SUB_0b10010110,"SUB_0b10010110"},
            {SUB_0b11010110,"SUB_0b11010110"},
            {SBC_0b10011xxx,"SBC_0b10011xxx"},
            {SBC_0b10011110,"SBC_0b10011110"},
            {SBC_0b11011110,"SBC_0b11011110"},
            {CP_0b10111xxx,"CP_0b10111xxx"},
            {CP_0b10111110,"CP_0b10111110"},
            {CP_0b11111110,"CP_0b11111110"},
            {INC_0b00xxx100,"INC_0b00xxx100"},
            {INC_0b00110100,"INC_0b00110100"},
            {DEC_0b00xxx101,"DEC_0b00xxx101"},
            {DEC_0b00110101,"DEC_0b00110101"},
            {AND_0b10100xxx,"AND_0b10100xxx"},
            {AND_0b10100110,"AND_0b10100110"},
            {AND_0b11100110,"AND_0b11100110"},
            {OR_0b10110xxx,"OR_0b10110xxx"},
            {OR_0b10110110,"OR_0b10110110"},
            {OR_0b11110110,"OR_0b11110110"},
            {XOR_0b10101xxx,"XOR_0b10101xxx"},
            {XOR_0b10101110,"XOR_0b10101110"},
            {XOR_0b11101110,"XOR_0b11101110"},
            {CCF_0b00111111,"CCF_0b00111111"},
            {SCF_0b00110111,"SCF_0b00110111"},
            {DAA_0b00100111,"DAA_0b00100111"},
            {CPL_0b00101111,"CPL_0b00101111"},
            {INC_0b00xx0011,"INC_0b00xx0011"},
            {DEC_0b00xx1011,"DEC_0b00xx1011"},
            {ADD_0b00xx1001,"ADD_0b00xx1001"},
            {ADD_0b11101000,"ADD_0b11101000"},
            {RLCA_0b00000111,"RLCA_0b00000111"},
            {RRCA_0b00001111,"RRCA_0b00001111"},
            {RLA_0b00010111,"RLA_0b00010111"},
            {RRA_0b00011111,"RRA_0b00011111"},
            {_0xCB,"_0xCB"},
            {JP_0b11000011,"JP_0b11000011"},
            {JP_0b11101001,"JP_0b11101001"},
            {JP_0b110xx010,"JP_0b110xx010"},
            {JR_0b00011000,"JR_0b00011000"},
            {JR_0b001xx000,"JR_0b001xx000"},
            {CALL_0b11001101,"CALL_0b11001101"},
            {CALL_0b110xx100,"CALL_0b110xx100"},
            {RET_0b11001001,"RET_0b11001001"},
            {RET_0b110xx000,"RET_0b110xx000"},
            {RETI_0b11011001,"RETI_0b11011001"},
            {RST_0b11xxx111,"RST_0b11xxx111"},
            {HALT,"HALT"},
            {STOP,"STOP"},
            {DI_0b11110011,"DI_0b11110011"},
            {EI_0b11111011,"EI_0b11111011"},
            {NOP_0b00000000,"NOP_0b00000000"}
    };

    std::unordered_map<Instruction, std::string_view> instruction_doctor_match = {
            {LD_0b01xxxyyy,"LD"},
            {LD_0b00xxx110,"LD"},
            {LD_0b01xxx110,"LD"},
            {LD_0b01110xxx,"LD"},
            {LD_0b00110110,"LD"},
            {LD_0b00001010,"LD"},
            {LD_0b00011010,"LD"},
            {LD_0b00000010,"LD"},
            {LD_0b00010010,"LD"},
            {LD_0b11111010,"LD"},
            {LD_0b11101010,"LD"},
            {LDH_0b11110010,"LDH"},
            {LDH_0b11100010,"LDH"},
            {LDH_0b11110000,"LDH"},
            {LDH_0b11100000,"LDH"},
            {LD_0b00111010,"LD"},
            {LD_0b00110010,"LD"},
            {LD_0b00101010,"LD"},
            {LD_0b00100010,"LD"},
            {LD_0b00xx0001,"LD"},
            {LD_0b00001000,"LD"},
            {LD_0b11111001,"LD"},
            {PUSH_0b11xx0101,"PUSH"},
            {POP_0b11xx0001,"POP"},
            {LD_0b11111000,"LD"},
            {ADD_0b10000xxx,"ADD"},
            {ADD_0b10000110,"ADD"},
            {ADD_0b11000110,"ADD"},
            {ADC_0b10001xxx,"ADD"},
            {ADC_0b10001110,"ADC"},
            {ADC_0b11001110,"ADC"},
            {SUB_0b10010xxx,"SUB"},
            {SUB_0b10010110,"SUB"},
            {SUB_0b11010110,"SUB"},
            {SBC_0b10011xxx,"SBC"},
            {SBC_0b10011110,"SBC"},
            {SBC_0b11011110,"SBC"},
            {CP_0b10111xxx,"CP"},
            {CP_0b10111110,"CP"},
            {CP_0b11111110,"CP"},
            {INC_0b00xxx100,"INC"},
            {INC_0b00110100,"INC"},
            {DEC_0b00xxx101,"DEC"},
            {DEC_0b00110101,"DEC"},
            {AND_0b10100xxx,"AND"},
            {AND_0b10100110,"AND"},
            {AND_0b11100110,"AND"},
            {OR_0b10110xxx,"OR"},
            {OR_0b10110110,"OR"},
            {OR_0b11110110,"OR"},
            {XOR_0b10101xxx,"XOR"},
            {XOR_0b10101110,"XOR"},
            {XOR_0b11101110,"XOR"},
            {CCF_0b00111111,"CCF"},
            {SCF_0b00110111,"SCF"},
            {DAA_0b00100111,"DAA"},
            {CPL_0b00101111,"CPL"},
            {INC_0b00xx0011,"INC"},
            {DEC_0b00xx1011,"DEC"},
            {ADD_0b00xx1001,"ADD"},
            {ADD_0b11101000,"ADD"},
            {RLCA_0b00000111,"RLCA"},
            {RRCA_0b00001111,"RRCA"},
            {RLA_0b00010111,"RLA"},
            {RRA_0b00011111,"RRA"},
            {_0xCB,"PREFIX"},
            {JP_0b11000011,"JP"},
            {JP_0b11101001,"JP"},
            {JP_0b110xx010,"JP"},
            {JR_0b00011000,"JR"},
            {JR_0b001xx000,"JR"},
            {CALL_0b11001101,"CALL"},
            {CALL_0b110xx100,"CALL"},
            {RET_0b11001001,"RET"},
            {RET_0b110xx000,"RET"},
            {RETI_0b11011001,"RETI"},
            {RST_0b11xxx111,"RST"},
            {HALT,"HALT"},
            {STOP,"STOP"},
            {DI_0b11110011,"DI"},
            {EI_0b11111011,"EI"},
            {NOP_0b00000000,"NOP"}
    };
};

#endif //GAMEBOY_CPU_H
