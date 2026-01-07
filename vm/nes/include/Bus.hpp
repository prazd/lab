#ifndef NES_BUS_HPP
#define NES_BUS_HPP
struct Bus {
    bool nmi_interrupt_flip_flop = false;
    unsigned char dma_source_high_byte{};

    struct PPUREG {
        unsigned char reg{};
        bool was_written{};
    };
    PPUREG ppuctrl{};
    PPUREG ppumask{};
    PPUREG ppustatus{};
    PPUREG oamaddr{};
    PPUREG oamdata{};
    PPUREG ppuscroll{};
    PPUREG ppuaddr{};
    PPUREG ppudata{};

    void write(unsigned short addr, unsigned char val);
    unsigned char read(unsigned short addr);
    unsigned char* read_ptr(unsigned short addr);
};
#endif //NES_BUS_HPP
