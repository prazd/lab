#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <linux/userfaultfd.h>

const int PAGE_SZ = 4096;

typedef struct {
    int fd;
    void* addr;
} Shared;
Shared shared;

typedef struct {
    void* addr;
    int uffd;
} S;

// TODO: handle > 1
void* uffd_handler(void* v){
    S* s_ptr = (S*)v;
    struct uffd_msg msg;
    ssize_t nread = read(s_ptr->uffd, &msg, sizeof(msg));
    if (nread <= 0){
        perror("read error");
        abort();
    }
    // Simple CoW
    if (msg.event == UFFD_EVENT_PAGEFAULT){
        puts("UFFD handle...");
        // TODO: finish
        // struct uffdio_writeprotect wp = {
        //     .range = {
        //         .start = (unsigned long)s_ptr->addr,
        //         .len = PAGE_SZ,
        //     },
        //     .mode = UFFDIO_WRITEPROTECT_MODE_DONTWAKE,
        // };
        // if (ioctl(s_ptr->uffd, UFFDIO_WRITEPROTECT, &wp) == -1){
        //     perror("ioctl uffd register error");
        //     abort();
        // }
        if (s_ptr->uffd) close(s_ptr->uffd);
        void* addr = mmap(s_ptr->addr, PAGE_SZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        if (addr == MAP_FAILED){perror("mmap failed"); abort();}
        assert(s_ptr->addr == addr);
        memcpy(s_ptr->addr, shared.addr, PAGE_SZ);
    }
    return NULL;
}

S create_from_shared(){
    S s;

    s.addr = mmap(NULL, PAGE_SZ, PROT_READ | PROT_WRITE, MAP_SHARED, shared.fd, 0);
    if (s.addr == MAP_FAILED){perror("mmap failed"); abort();}

    int uffd = syscall(__NR_userfaultfd,  UFFD_USER_MODE_ONLY);
    if (uffd == -1) {perror("userfaultfd err"); abort();}
    s.uffd = uffd;

    struct uffdio_api uapi = {
        .api = UFFD_API,
        .features = 0,
    };

    if (ioctl(uffd, UFFDIO_API, &uapi) == -1){
        perror("ioctl uffd api error");
        abort();
    }

    struct uffdio_register uffdr = {
        .range = {
            .start = (unsigned long)s.addr,
            .len = PAGE_SZ,
        },
        .mode = UFFDIO_REGISTER_MODE_WP,
    };

    
    if (ioctl(uffd, UFFDIO_REGISTER, &uffdr) == -1){
        perror("ioctl uffd register error");
        abort();
    }

    struct uffdio_writeprotect wp = {
        .range = {
            .start = (unsigned long)s.addr,
            .len = PAGE_SZ,
        },
        .mode = UFFDIO_WRITEPROTECT_MODE_WP,
    };
    
    if (ioctl(uffd, UFFDIO_WRITEPROTECT, &wp) == -1){
        perror("ioctl uffd register error");
        abort();
    }

    return s;
}

void __attribute__((constructor)) init(){
    shared.fd = memfd_create("uffd_example", 0);
    ftruncate(shared.fd, PAGE_SZ);
    shared.addr = mmap(NULL, PAGE_SZ, PROT_READ | PROT_WRITE, MAP_SHARED, shared.fd, 0);
    if (shared.addr == MAP_FAILED){perror("mmap failed"); abort();}
    memset(shared.addr, 0x90, 128);
}

int main(){
    S s = create_from_shared();

    pthread_t t;
    if (pthread_create(&t, NULL, uffd_handler, (void*)&s) != 0){
        perror("pthread create error");
        return -1;
    }

    assert(*((unsigned char*)s.addr) == 0x90);
    printf("S[0]:%d\n", *((unsigned char*)s.addr));

    *((unsigned char*)s.addr) = 0x91;
    assert(*((unsigned char*)s.addr) == 0x91);
    printf("S[0]:%d\n", *((unsigned char*)s.addr));

    pthread_join(t, NULL);
    if (s.uffd > 0) close(s.uffd);
    munmap(shared.addr, PAGE_SZ);
    munmap(s.addr, PAGE_SZ);
}
