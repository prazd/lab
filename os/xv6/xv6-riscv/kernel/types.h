typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;

unsigned to_little_endian32(unsigned val);
unsigned to_little_endian64(unsigned long val);
unsigned char cmp_str(const char* str1, const char* str2, int sz);