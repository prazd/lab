#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#define IOCTL_CALL 'p'
#define IOCTL_READ_RDGSBASE_HANDLE \
    _IOWR(IOCTL_CALL, 3, unsigned long)  \    

int main(){
   int fd = open("/dev/ioctl_call", O_RDONLY);
   unsigned long res = 0;
   if (ioctl(fd, IOCTL_READ_RDGSBASE_HANDLE, &res) == -1) {
        perror("IOCTL_READ_RDGSBASE_HANDLE error"); 
        return -1;
    }
   printf("Res:%p\n", (void*)res);
   return 0;
}
