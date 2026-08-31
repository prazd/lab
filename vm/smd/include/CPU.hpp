#ifndef SMD_CPU_HPP
#define SMD_CPU_HPP
#include "BUS.hpp"
struct MC68K {
    // SPR
    unsigned pc{};
    // User stack pointer
    unsigned usp{};
    // Supervisor stack pointer
    unsigned ssp{};
    // Vector base register
    unsigned vbr{};
    // Alternate function code registers
    unsigned sfc{};
    unsigned dfc{};
    
    // Status register
    unsigned short sreg{};
    enum SREG: unsigned short {
        Carry=0x0,Overflow=0x2,Zero=0x4,Negative=0x8,Extend=0x10,
        Interrupt0=0x100,Interrupt1=0x200,Interrupt2=0x400,
        State0=0x1000,State1=0x2000,TraceMode0=0x4000,TraceMode1=0x8000
    };
    inline void set_sreg(SREG s, bool val){
        sreg &= ~(1 << s);
        sreg |= (val << s);
    }
    inline bool get_srge(SREG s){
        return sreg & s;
    }

    // GPR
    // Data registers
    enum DREG: unsigned char {D0,D1,D2,D3,D4,D5,D6,D7};
    unsigned dregs[8]{};
    
    // Address registers
    enum AREG: unsigned char {A0,A1,A2,A3,A4,A5,A6,A7};
    unsigned aregs[8]{};


    enum AddressMode: unsigned char{
        DataRegisterDirectMode,
        AddressRegisterDirectMode,
        AddressRegisterIndirectMode,
        AddressRegisterIndirectPostIncMode,
        AddressRegisterIndirectPreDecMode,
        AddressRegisterIndirectDisplacementMode,
        AddressRegisterIndirectIndexMode,
        ProgramCounterInderectDisplacementMode,
        ProgramCounterInderectIndexMode,
        AbsoluteShort,
        AbsoluteLong,
        Immediate,
        Unknown,
    };

    AddressMode to_adress_mode(unsigned char);

    enum Instruction {
        ABCD,ADD,ADDA,ADDI,ADDQ,ADDX,AND,ANDI,ASL,ASR,
        BCC,BCHG,BCLR,BRA,BSET,BSR,BTST,
        CHK,CLR,CMP,CMPA,CMPI,CMPM,
        DBcc, DIVS, DIVSL,DIVU,DIVUL,
        EOR,EORI,EORItoCCR,EXG,EXT,
        ILLEGAL,
        JMP,JSR,
        LEA,LINKw,LINKl,LSL,LSR,
        MOVE,MOVEA,MOVEtoCCR,MOVEfromSR,MOVEM,MOVEP,MOVEQ,MULS,MULU,
        NBCD,NEG,NEGX,NOP,NOT,
        OR,ORI,ORItoCCR,
        PEA,
        ROL,ROR,ROXL,ROXR,RTM,RTR,RTS,
        SBCD,Scc,SUB,SUBA,SUBI,SUBQ,SUBX,SWAP,
        TAS,TRAP,TRAPV,TST,
        UNLK,
    };

    struct DecodedInfo {
        Instruction i;
        bool address_reg{};
        unsigned char rx{};
        unsigned char ry{};
        unsigned char d1{};
        unsigned char d2{};
        unsigned char d3{};
        unsigned char d4{};
        unsigned mask{};
        unsigned sign_bit{};
        AddressMode addr_mode{};
        unsigned char effective_address{};
    };

    unsigned& read_operand(unsigned char effective_address);
    DecodedInfo decode(unsigned short);

    void exec(DecodedInfo di, BUS& bus);
};
#endif