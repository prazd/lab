#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

typedef struct {
    uintptr_t regs[8];
    int err;
} DRState;

#define IOCTL_READ_DR 'p'
#define IOCTL_READ_DR_HANDLE \
    _IOWR(IOCTL_READ_DR, 1, DRState)


// in gdb - watch -l variable
int main(){
   int fd = open("/dev/ioctl_read_dr", O_RDONLY);
   DRState drs;
   int in = 0;
   while(1){
    scanf("%d", &in);
    if (in == 0) 
        break;
    if (ioctl(fd, IOCTL_READ_DR_HANDLE, &drs) == -1){
      printf("ERROR\n");
      close(fd);
      return 1;
    }
    for (size_t i = 0; i < 8; ++i)
        printf("DR[%zu]:%p\n",i,(void*)drs.regs[i]);
   }
   printf("done!\n"); 
   return 0;
}