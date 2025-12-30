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

};
