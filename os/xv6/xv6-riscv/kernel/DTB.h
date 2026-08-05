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
extern uint64 TMP;

inline unsigned to_little_endian(unsigned val) {
  unsigned res = 0;
  unsigned char* r_ptr = (unsigned char*)&res;
  unsigned char* val_ptr = (unsigned char*)&val;
  r_ptr[0] = val_ptr[3];
  r_ptr[1] = val_ptr[2];
  r_ptr[2] = val_ptr[1];
  r_ptr[3] = val_ptr[0];
  return res;
}

inline unsigned char cmp_str(const char* str1, const char* str2, int sz){
  for (int i = 0; i < sz; ++i){
    if (str1[i] != str2[i]) return 0;
  }
  return 1;
}


inline void parse_dtb(){
  unsigned char* dt_struct_ptr = ((unsigned char*)DTB_ADDRESS) + to_little_endian(dtb_header.off_dt_struct);
  unsigned* check = (unsigned*)dt_struct_ptr;
  unsigned cur = to_little_endian(*check);
  unsigned char found = 0;
  while (cur != FDT_END){
     if (cmp_str((char*)(check+1), "memory", 6)) {
        found = 1;
        break;
     } else {
        ++check;
        while(to_little_endian(*check) != FDT_BEGIN_NODE) {
          ++check;
          if (to_little_endian(*check) == FDT_END) break;
        }
        cur = to_little_endian(*check);
     }
  }

  if (found){
    char* dt_strings_ptr = ((char*)DTB_ADDRESS) + to_little_endian(dtb_header.off_dt_strings);
//    unsigned char found_reg = 0;
    check+=5; // memo'ry@8'0000'000(\0)'
//     FDT_PROP
//     len
    check+=2;
// TODO: finish
//    while(!found_reg){
//      if (cmp_str(dt_strings_ptr + to_little_endian(*check), "reg", 3)){
//        found_reg = 1;
//        break;
//      } else {
//        while(to_little_endian(*check) != FDT_PROP){++check;}
//        check+=2;
//      }
//    }
    TMP = *((uint64*)(dt_strings_ptr + to_little_endian(*check)));
  }
}

inline void set_dtb_header(){
  unsigned char* addr = (unsigned char*)DTB_ADDRESS;
  unsigned char* ptr = (unsigned char*)&dtb_header;
  for (int i = 0, e = sizeof(dtb_header); i != e; ++i)
    *ptr++ = *addr++;
  parse_dtb();
}

inline uint64 get_dtb_start() {
//  unsigned char* dt_struct_ptr = ((unsigned char*)DTB_ADDRESS) + dtb_header.off_dt_struct;
//  unsigned* check = (unsigned*)dt_struct_ptr;
  return TMP;
}