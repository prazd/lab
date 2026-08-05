#include "kernel/types.h"
#include "user/user.h"

char buf[9];

int main(int argc, char *argv[]){
  uint64 res = dtbs();
  printf("%p\n", (void*)res);
  memcpy(buf, &res, sizeof(uint64));
  printf("%s\n", buf);
}