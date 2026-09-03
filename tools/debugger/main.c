#include <stdio.h>
#include <unistd.h>
int main(){
   printf("Start process...\n");

   printf("l5\n");
   printf("l6\n");
   for (int i = 0; i < 3; ++i){
      printf("i1:%d", i);
      printf("i2:%d", i);
   }
   printf("!!!Finish!!!\n");
}
