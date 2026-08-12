#include "types.h"
unsigned to_little_endian32(unsigned val) {
  unsigned res = 0;
  unsigned char* r_ptr = (unsigned char*)&res;
  unsigned char* val_ptr = (unsigned char*)&val;
  r_ptr[0] = val_ptr[3];
  r_ptr[1] = val_ptr[2];
  r_ptr[2] = val_ptr[1];
  r_ptr[3] = val_ptr[0];
  return res;
}

unsigned long to_little_endian64(unsigned long val) {
  unsigned long res = 0;
  unsigned char* r_ptr = (unsigned char*)&res;
  unsigned char* val_ptr = (unsigned char*)&val;
  r_ptr[0] = val_ptr[7];
  r_ptr[1] = val_ptr[6];
  r_ptr[2] = val_ptr[5];
  r_ptr[3] = val_ptr[4];
  r_ptr[4] = val_ptr[3];
  r_ptr[5] = val_ptr[2];
  r_ptr[6] = val_ptr[1];
  r_ptr[7] = val_ptr[0];
  return res;
}

unsigned char cmp_str(const char* str1, const char* str2, int sz){
  for (int i = 0; i < sz; ++i){
    if (str1[i] != str2[i]) return 0;
  }
  return 1;
}

static uint64 state = 0;
void set_rand_seed(uint64 seed){
  state = seed;
}

uint64 get_rand_state(){
  return state;
}


uint64 xorshift(){
    uint64 x = state;
    x ^= x >> 12;
    x ^= x >> 25;
    x ^= x >> 27;
    state = x;
    return x * 2685821657736338717;
}