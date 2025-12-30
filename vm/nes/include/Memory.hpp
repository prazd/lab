// https://www.nesdev.org/NESDoc.pdf (page 10)
// $0000-$00FF - used by certain addressing modes to allow quicker execution 
// $0000-$07FF are mirrored three times at $0800-$1FFF (for example, any data written to $0000 will also be written to $0800, $1000 and $1800)
// $2000-$401F - I/O regs
// $2000-$2007 are mirrored every 8 bytes in the region $2008-$3FFF and the remaining registers follow this mirroring

struct Memory {
    unsigned char map[0x10000];
};
