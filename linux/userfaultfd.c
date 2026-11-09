#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <stdatomic.h>
#include <linux/userfaultfd.h>

const int PAGE_SZ = 4096;
atomic_bool finish;

typedef struct {
    int fd;
    void* addr;
} Shared;
Shared shared;

typedef struct {
    void* addr;
    int uffd;
} S;

typedef struct {
    size_t sz;
    S arr[3];
} Pool;

void pool_add_from_share(Pool* p){
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

    p->arr[p->sz++] = s;
}

// TODO: handle > 1
void* uffd_handler(void* v){
    Pool* pool_ptr = (Pool*)v;

    fd_set uffd_set;

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    
    int max_fd = 0;

    for(;;){
        FD_ZERO(&uffd_set);
        for (int i = 0, e =  pool_ptr->sz; i != e; ++i)
            if (pool_ptr->arr[i].uffd > max_fd) max_fd = pool_ptr->arr[i].uffd;
        for (int i = 0, e = pool_ptr->sz; i != e; ++i)
            if (pool_ptr->arr[i].uffd >= 0) FD_SET(pool_ptr->arr[i].uffd, &uffd_set);    


        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        switch(select(max_fd + 1, &uffd_set, NULL, NULL, &timeout)){
            case -1: perror("select error"); return NULL;
            case 0:  {
                puts("select timeout"); 
                if (atomic_load(&finish)) return NULL;
                break;
            }
            default: {
                for (int i = 0, e = pool_ptr->sz; i != e; ++i){
                    if (pool_ptr->arr[i].uffd >=0 && FD_ISSET(pool_ptr->arr[i].uffd, &uffd_set)){
                        puts("ready!!!");
                        struct uffd_msg msg;
                        ssize_t nread = read(pool_ptr->arr[i].uffd, &msg, sizeof(msg));
                        if (nread <= 0){
                            perror("read error");
                            abort();
                        }
                        // Simple CoW
                        if (msg.event == UFFD_EVENT_PAGEFAULT){
                            puts("UFFD handle...");
                            
                            void* addr = mmap(NULL, PAGE_SZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                            if (addr == MAP_FAILED){perror("mmap failed"); abort();}
                            memcpy(addr, shared.addr, PAGE_SZ);
                            if (mremap(addr, PAGE_SZ, PAGE_SZ, MREMAP_FIXED | MREMAP_MAYMOVE, pool_ptr->arr[i].addr) == MAP_FAILED){
                                perror("mremap failed");
                                return NULL;
                            }
                            if (pool_ptr->arr[i].uffd) close(pool_ptr->arr[i].uffd);
                            pool_ptr->arr[i].uffd = -1;
                        }
                    }
                }
            }
        }
    }

    
    return NULL;
}

void __attribute__((constructor)) init(){
    shared.fd = memfd_create("uffd_example", 0);
    ftruncate(shared.fd, PAGE_SZ);
    shared.addr = mmap(NULL, PAGE_SZ, PROT_READ | PROT_WRITE, MAP_SHARED, shared.fd, 0);
    if (shared.addr == MAP_FAILED){perror("mmap failed"); abort();}
    memset(shared.addr, 0x90, 128);
}

int main(){
    Pool p = {.sz = 0};
    pool_add_from_share(&p);

    pthread_t t;
    if (pthread_create(&t, NULL, uffd_handler, (void*)&p) != 0){
        perror("pthread create error");
        return -1;
    }

    assert(*((unsigned char*)p.arr[0].addr) == 0x90);
    printf("S[0]:%d\n", *((unsigned char*)p.arr[0].addr));

    *((unsigned char*)p.arr[0].addr) = 0x91;
    assert(*((unsigned char*)p.arr[0].addr) == 0x91);
    printf("S[0]:%d\n", *((unsigned char*)p.arr[0].addr));

    atomic_store(&finish, true);
    pthread_join(t, NULL);
    for (size_t i = 0, e = p.sz; i != e; ++i) {
        if (p.arr[i].uffd > 0) close(p.arr[i].uffd);
        if (p.arr[i].addr != shared.addr) munmap(p.arr[i].addr, PAGE_SZ);
    }
    munmap(shared.addr, PAGE_SZ);
}
