typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;

extern unsigned to_little_endian32(unsigned val);
extern unsigned long to_little_endian64(unsigned long val);
extern unsigned char cmp_str(const char* str1, const char* str2, int sz);

extern void set_rand_seed(uint64);
extern uint64 xorshift();
extern uint64 get_rand_state();