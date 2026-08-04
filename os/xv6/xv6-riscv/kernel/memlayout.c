#include "memlayout.h"
unsigned long KERNBASE = 0x80000000L;
unsigned long PHYSTOP = (0x80000000L + 128 * 1024 * 1024);