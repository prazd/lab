#include "types.h"
#include <assert.h>
struct DTBHeader {
  unsigned magic;
  unsigned total_size;
  unsigned off_dt_struct;
  unsigned off_dt_strings;
  unsigned off_mem_rsvmap;
  unsigned version;
  unsigned last_comp_version;
  unsigned boot_cpuid_phys;
  unsigned size_dt_strings;
  unsigned size_dt_struct;
};
enum DTBToken {
  FDT_BEGIN_NODE=0x1,
  FDT_END_NODE=0x2,
  FDT_PROP=0x3,
  FDT_NOP=0x4,
  FDT_END=0x9
};
extern uint64 DTB_ADDRESS;
extern struct DTBHeader dtb_header;
extern uint64 ADDRBEGIN;
extern uint64 ADDREND;

static inline void parse_dtb(){
  unsigned char* dt_struct_ptr = ((unsigned char*)DTB_ADDRESS) + to_little_endian32(dtb_header.off_dt_struct);
  unsigned* dt_struct_u32_ptr = (unsigned*)dt_struct_ptr;
  unsigned cur = to_little_endian32(*dt_struct_u32_ptr);
  unsigned char found = 0;
  while (cur != FDT_END){
     if (cmp_str((char*)(dt_struct_u32_ptr+1), "memory", 6)) {
        found = 1;
        break;
     } else {
        ++dt_struct_u32_ptr;
        while(to_little_endian32(*dt_struct_u32_ptr) != FDT_BEGIN_NODE) {
          ++dt_struct_u32_ptr;
          if (to_little_endian32(*dt_struct_u32_ptr) == FDT_END) break;
        }
        cur = to_little_endian32(*dt_struct_u32_ptr);
     }
  }



  if (found){
    const char* dt_strings_ptr = ((const char*)DTB_ADDRESS) + to_little_endian32(dtb_header.off_dt_strings);
    dt_struct_u32_ptr+=5; // memo'ry@8'0000'000(\0)'
    int reg_found = 0;
//     FDT_PROP
//     len
    dt_struct_u32_ptr+=2;
    while(to_little_endian32(*dt_struct_u32_ptr) != FDT_END) {
      if (cmp_str(dt_strings_ptr + to_little_endian32(*dt_struct_u32_ptr), "reg", 3)) {
        reg_found = 1;
        break;
      } else {
        while(to_little_endian32(*dt_struct_u32_ptr) != FDT_PROP){++dt_struct_u32_ptr;}
        dt_struct_u32_ptr+=2;
      }
    }
    if (reg_found){
      // 0x80000000 - base
      // 0x8000000 - sz
      ADDRBEGIN = to_little_endian64(*(((uint64*)(dt_struct_u32_ptr+1))));
      ADDREND = ADDRBEGIN + to_little_endian64(*(((uint64*)(dt_struct_u32_ptr+1)) + 1));
    }
  }
}

static inline void set_dtb_header(){
  unsigned char* addr = (unsigned char*)DTB_ADDRESS;
  unsigned char* ptr = (unsigned char*)&dtb_header;
  for (int i = 0, e = sizeof(dtb_header); i != e; ++i)
    *ptr++ = *addr++;
}

inline uint64 get_dtb_start() {
  return ADDRBEGIN;
}