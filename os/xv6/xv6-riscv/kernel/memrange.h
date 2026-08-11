// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80000000 to PHYSTOP.
extern unsigned long KERNBASE;
extern unsigned long PHYSTOP;
void update_addr_range(unsigned long, unsigned long);