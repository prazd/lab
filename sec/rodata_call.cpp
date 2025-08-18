#include <iostream>

// 100003e61: 55                          	push	rbp
// 100003e62: 48 89 e5                    	mov	rbp, rsp
// 100003e65: 89 7d fc                    	mov	dword ptr [rbp - 4], edi
// 100003e68: 89 75 f8                    	mov	dword ptr [rbp - 8], esi
// 100003e6b: 8b 55 fc                    	mov	edx, dword ptr [rbp - 4]
// 100003e6e: 8b 45 f8                    	mov	eax, dword ptr [rbp - 8]
// 100003e71: 01 d0                       	add	eax, edx
// 100003e73: 5d                          	pop	rbp
// 100003e74: c3                          	ret

constexpr unsigned char code[] = {
    0x55,
    0x48, 0x89, 0xe5,
    0x89, 0x7d, 0xfc,
    0x89, 0x75, 0xf8,
    0x8b, 0x55, 0xfc,
    0x8b, 0x45, 0xf8,
    0x01, 0xd0,
    0x5d,
    0xc3
};

int main() {
    auto add = (int(*)(int,int))code; // UB
    std::cout << add(20,22);
}