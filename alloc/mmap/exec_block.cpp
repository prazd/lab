#include <iostream>
#include <sys/mman.h>
#include <cassert>
#include <array>
#include <cstring>

std::array<unsigned char, 20> sum_code = {
    0x55,              // 	push	rbp
    0x48, 0x89, 0xe5,  //   mov	    rbp, rsp
    0x89, 0x7d, 0xfc,  //   mov	dword ptr [rbp - 4], edi
    0x89, 0x75, 0xf8,  //   mov	dword ptr [rbp - 8], esi
    0x8b, 0x55, 0xfc,  //   mov	edx, dword ptr [rbp - 4]
    0x8b, 0x45, 0xf8,  //   mov	eax, dword ptr [rbp - 8]
    0x01, 0xd0,        //   add	eax, edx
    0x5d,              //  	pop	rbp
    0xc3,              //   ret
};

std::array<unsigned char, 19> mul_code = {
    0x55,                   // 	 push	rbp
    0x48, 0x89, 0xe5,       //   mov	rbp, rsp
    0x89, 0x7d, 0xfc,       //   mov	dword ptr [rbp - 4], edi
    0x89, 0x75, 0xf8,       //   mov	dword ptr [rbp - 8], esi
    0x8b, 0x45, 0xfc,       //   mov	eax, dword ptr [rbp - 4]
    0x0f, 0xaf, 0x45, 0xf8, //   imul	eax, dword ptr [rbp - 8]
    0x5d,                   //   pop	rbp
    0xc3,                   //   ret
};

template <typename FuncPtr, typename Container>
struct ExecBlock {
    static_assert(std::is_same_v<typename Container::value_type, unsigned char>);
    using ElementType = unsigned char;
    size_t count{};
    void* fptr = nullptr;
    
    ExecBlock(const ExecBlock&) = delete;
    ExecBlock& operator=(const ExecBlock&) = delete;
    
    ExecBlock(const Container& container): count(std::size(container) * sizeof(ElementType)),
        fptr(mmap(nullptr, count, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) {
        if (fptr == MAP_FAILED) {
            perror("mmap");
            throw std::bad_alloc();
        }

        memcpy(fptr, (void*)container.data(), count);
        if (mprotect(fptr, count, PROT_READ | PROT_EXEC) != 0) {
            perror("mprotect");
            munmap(fptr, count);
            throw std::bad_alloc();
        }
    }

    template <typename ... Args>
    void execute(Args&&... arg) {
        ((FuncPtr)fptr)(std::forward<Args>(arg)...);
    }

    template <typename ... Args>
    auto execute_r(Args&&... arg) {
        return ((FuncPtr)fptr)(std::forward<Args>(arg)...);
    }

    ~ExecBlock() {
        munmap(fptr, count);
    }
};

int main() {
    ExecBlock<int(*)(int, int), decltype(sum_code)> sum(sum_code);
    assert(sum.execute_r(10,20) == 30);

    ExecBlock<int(*)(int, int), decltype(mul_code)> mul(mul_code);
    assert(mul.execute_r(10,20) == 200);
}