#include <iostream>

// 16-bit vm:
// 0 0 0 1 | 0 0 1 | 0 0 1 | 0 | 0 0 0 0 0 
//  opcode   PARAM1  PARAM2        PARAM3
class VM {    
    enum Register {R0, R1, R2, R3, R4, R5, R6, R7, RPC, RCND, RCNT};
    static constexpr unsigned short PC_START = 0x3000;
    // todo: check
    unsigned short reg[RCNT+1]{};
    unsigned short mem[std::numeric_limits<unsigned short>::max()+1]{};

    enum last_operation_flags {FP = 1 << 0, FZ = 1 << 1, FN = 1 << 2};
    void uf(Register r) {
        if (reg[r] == 0) reg[RCND] = FZ;
        else if (reg[r] >> 15) reg[RCND] = FN;
        else reg[RCND] = FP;
    }

    bool fmm5(unsigned short i) {
        return  (i >> 5) & 1;
    }

    int getDR1(unsigned short i) {
        return (i >> 9) & 0b111; // 7
    }

    int getSR1(unsigned short i) {
        return (i >> 6) & 0b111; // 7
    }

    int getSR2(unsigned short i) {
        return i & 0b111; // 7
    }

    int getIMM(unsigned short i) {
        return i & 0b11111; // 31 
    }

    int getOffset9(unsigned short i) {
        return i & 0b111111111; // 511
    }

    unsigned short sext(unsigned short n, int b) {
        return (n>>(b-1) & 1) ? (n | (0xFFFF << b)) : n;
    }


    void br(unsigned short){std::cout << "BR" << "\n";}

    // NOTE: DR stands for Destination Register. SR stands for Source Register

    // add1/and1
    // 0 0 0 1 | . . . | . . . | 0 0 0 |  . . . 
    //  opcode   DR1      SR1              SR2

    // add2/and2
    // 0 0 0 1 | . . . | . . . | 1 |  . . . . . 
    //  opcode   DR1      SR1            IMM5 (is a positive or negative number on 5 bits)
    void addop(unsigned short i){
        reg[getDR1(i)] = reg[getSR1(i)] + (fmm5(i) ? sext(i, 5) : reg[getSR2(i)]);
        uf(getDR1(i));
    }

    void andop(unsigned short i){
        reg[getDR1(i)] = reg[getSR1(i)] & (fmm5(i) ? sext(i, 5) : reg[getSR2(i)]);
        uf(getDR1(i));
    }

    // ld - is an instruction to load data from a main memory location to a destination register, DR1;
    // The memory location is obtained by adding to the RPC register an offset value. 
    // Calling ld doesn’t modify the RPC; we use RPC just as a referencing point

    // 0 0 0 1 | . . . | . . . . . . . . . 
    //  opcode   DR1         OFFSET9
    void ld(unsigned short i){
        reg[getDR1(i)] = readmem(reg[RPC] + getOffset9(i));
        uf(getDR1(i));
    }

    void ldi(unsigned short i){
        reg[DR(i)] = readmem(readmem(reg[RPC]+POFF9(i)));
        uf(getDR1(i));
    }

    // TODO:
    


    void st(unsigned short){std::cout << "ST" << "\n";}
    void jsr(unsigned short){std::cout << "JSR" << "\n";}
    
    void ldr(unsigned short){std::cout << "LDR" << "\n";}
    void str(unsigned short){std::cout << "STR" << "\n";}
    void rti(unsigned short){std::cout << "RTI" << "\n";}
    void nott(unsigned short){std::cout << "NOT" << "\n";}
    
    void sti(unsigned short){std::cout << "STI" << "\n";}
    void jmp(unsigned short){std::cout << "JMP" << "\n";}
    void res(unsigned short){std::cout << "RES" << "\n";}
    void lea(unsigned short){std::cout << "LEA" << "\n";}
    void trap(unsigned short){std::cout << "TRAP" << "\n";}

    using ifunc = void (VM::*)(unsigned short);
    // instructions: {br, add, ld, st, jsr, and, ldr, str, rti, not, ldi, sti, jmp, res, lea, trap};
    ifunc instructions[16]{
        &VM::br, &VM::addop, &VM::ld, &VM::st, &VM::jsr,
        &VM::andop, &VM::ldr, &VM::str, &VM::rti, 
        &VM::nott, &VM::ldi, &VM::sti, &VM::jmp, 
        &VM::res, &VM::lea, &VM::trap
    };

    int get_opcode(unsigned short ins) {
        return ins>>12;
    }

public:
    VM() = default;
    unsigned short readmem(int address) const {
        return mem[address];
    }
    void writemem(int address, unsigned short data) {
        mem[address] = data;
    }

    void call(unsigned short ins) {
        (this->*instructions[get_opcode(ins)])(ins);
    }
};

int main() {
    VM vm;
    for(int i = 0; i < 16; ++i)
        vm.call(i<<12);
}