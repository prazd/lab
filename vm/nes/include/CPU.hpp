// https://www.nesdev.org/NESDoc.pdf (page 12-13)
struct CPU {
    // SPR
    unsigned short pc{}; 
    unsigned char sp{};

    struct ProcessorStatusReg {
        // | N | V |   | B | D | I | Z | C |
        // | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        unsigned char reg{};
        enum Flag: unsigned char {
            C = 0,
            Z = 1,
            I = 2,
            D = 3,
            B = 4,
            V = 6,
            N = 7
        };
        bool read_p(Flag);
        void set_p(Flag flag, bool value);
} p;
                       
    // GPR
    unsigned char a{};
    unsigned char x{};
    unsigned char y{};

    // https://www.nesdev.org/wiki/CPU_addressing_modes
    enum AddressingMode {
        ZeroPage,
        ZeroPageX,
        ZeroPageY,
        Absolute,
        AbsoluteX,
        AbsoluteY,
        IndexedIndirectX,
        IndexedIndirectY,
        IndexedZeroPage,
        IndexedAbsolute,
        Indirect,
        Implied,
        Accumulator,
        Immediate,
        Relative,
        IndirectIndexed,
    };

    enum Instruction: unsigned char {
        ADC, AND, ASL,BCC, BCS,	BEQ, BIT, BMI, BNE,	BPL, BRK, BVC, BVS,	CLC,
        CLD, CLI, CLV,CMP, CPX,	CPY, DEC, DEX, DEY,	EOR, INC, INX, INY,	JMP,
        JSR, LDA, LDX,LDY, LSR,	NOP, ORA, PHA, PHP,	PLA, PLP, ROL, ROR,	RTI,
        RTS, SBC, SEC,SED, SEI,	STA, STX, STY, TAX,	TAY, TSX, TXA, TXS,	TYA,
    };

    struct DecodedInstruction {
        AddressingMode addr_mode;
        Instruction i;
        unsigned char bytes;
        unsigned char cycles;
    };

    DecodedInstruction decode(unsigned char v);
    void execute(Instruction i);
};
