#include "memlayout.h"
unsigned long KERNBASE = 0;
unsigned long PHYSTOP = 0;

void update_addr_range(unsigned long begin, unsigned long end){
  KERNBASE = begin;
  PHYSTOP = end;
}