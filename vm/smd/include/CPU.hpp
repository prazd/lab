#ifndef SMD_CPU_HPP
#define SMD_CPU_HPP
struct MC68K {
    // SPR
    unsigned pc{};
    // User stack pointer
    unsigned usp{};
    // Supervisor stack pointer
    unsigned ssp{};
    
    // Status register
    unsigned short sreg{};
    enum SREG: unsigned short {
        Carry=0x0,Overflow=0x2,Zero=0x4,Negative=0x8,Extend=0x10,
        Interrupt0=0x100,Interrupt1=0x200,Interrupt2=0x400,
        SupervisorState=0x2000,TraceMode=0x8000
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
    enum DREG {D0,D1,D2,D3,D4,D5,D6,D7};
    unsigned dregs[8]{};
    
    // Address registers
    enum AREG {A0,A1,A2,A3,A4,A5,A6,A7};
    unsigned aregs[8]{};


    enum AddressingModes{
        RegisterDirect,
        RegisterIndirect,
        Absolute,
        ProgramCounterRelative,
        Immediate,
        Implied,
    };

};
#endif