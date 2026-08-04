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
extern uint64 DTB_ADDRESS;
extern struct DTBHeader dtb_header;

inline void set_dtb_header(){
  unsigned char* addr = (unsigned char*)DTB_ADDRESS;
  unsigned char* ptr = (unsigned char*)&dtb_header;
  for (int i = 0, e = sizeof(dtb_header); i != e; ++i)
    *ptr++ = *addr++;
}

inline uint64 get_dtb_start() {
  return (uint64)dtb_header.magic;
}