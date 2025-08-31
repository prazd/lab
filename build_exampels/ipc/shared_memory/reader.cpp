#include <iostream>
#include <fcntl.h>    
#include <sys/mman.h> 
#include <unistd.h>   
#include <cstring>

int main() {
    auto name = "/obj";     
    const size_t sz = 4096;

    int shm_fd = shm_open(name, O_RDONLY, 0666);
    if (shm_fd == -1) return -1;

    void* ptr = mmap(0, sz, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) return -1;

    std::cout << (const char*)ptr << '\n';
    std::cout << "Read\n";
    
    close(shm_fd);
    shm_unlink(name);
}