#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#define IOCTL_CALL 'p'
#define IOCTL_RDPMC_HANDLE \
    _IOWR(IOCTL_CALL, 0, unsigned long)

// TODO: test
int main(){
   int fd = open("/dev/ioctl_call", O_RDONLY);
   unsigned long msr_req = (1 << 30);
   if (ioctl(fd, IOCTL_RDPMC_HANDLE, &msr_req) == -1){
      printf("ERROR\n");
      close(fd);
      abort();
    }
    printf("RDPMC:%lx\n", msr_req);
    close(fd);

   return 0;
}