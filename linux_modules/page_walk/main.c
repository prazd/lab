#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>

#define IOCTL_PAGE_WALK 'p'
#define IOCTL_PAGE_WALK_HANDLE \
   _IOWR(IOCTL_PAGE_WALK, 1, page_walk_req)

typedef struct {
    unsigned long va;
    long res;
} page_walk_req;

// Run:
// # sysctl -w vm.nr_hugepages=1
int main(){
   int fd = open("/dev/ioctl_page_walk", O_RDONLY);
   long var = 256;
   page_walk_req req;
   req.va = (long)(&var);
   if (ioctl(fd, IOCTL_PAGE_WALK_HANDLE, &req) == -1){
        printf("ERROR\n");
	close(fd);
	return 1;
   }
   printf("VAR VALUE:%lu\n", var);
   printf("RESULT:%lu\n", req.res);

   // Huge page
   void* hp_addr = mmap(NULL, 1 << 21, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
   if (hp_addr == MAP_FAILED) {printf("Huge page mmap failed\n"); return 1;}
   long* hp_val_ptr = (long*)(hp_addr);
   *hp_val_ptr = 512;
   
   req.va = (unsigned long)hp_addr;
   if (ioctl(fd, IOCTL_PAGE_WALK_HANDLE, &req) == -1){
        printf("ERROR\n");
        munmap(hp_addr, 1 << 21);
        close(fd);
        return 1;
   }
    printf("HUGE PAGE VAR VALUE:%lu\n", *hp_val_ptr);
    printf("HUGE PAGE RESULT:%lu\n", req.res);

   munmap(hp_addr, 1 << 21);
   close(fd);
   return 0;
}