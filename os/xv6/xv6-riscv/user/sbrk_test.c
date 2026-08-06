#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]){
  printf("Start:\n");
  printptes();
  printf("Proc sz:%lu\n", getpsz());
  printf("\n\n");


  printf("SBRK 1\n");
  sbrk(1);
  printptes();
  printf("Proc sz:%lu\n", getpsz());
  printf("\n\n");

  printf("SBRK 4096\n");
  sbrk(4096);
  printptes();
  printf("Proc sz:%lu\n", getpsz());
  printf("\n\n");

}