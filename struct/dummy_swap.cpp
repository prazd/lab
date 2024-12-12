#include <iostream>
#include <concepts>
#include <cassert>


// void swap<int>(int&, int&):
//15//         push    rbp
    //         mov     rbp, rsp
    //         mov     qword ptr [rbp - 8], rdi
    //         mov     qword ptr [rbp - 16], rsi
    //         mov     rax, qword ptr [rbp - 16]
    //         mov     eax, dword ptr [rax]
    //         mov     dword ptr [rbp - 20], eax
    //         mov     rax, qword ptr [rbp - 8]
    //         mov     ecx, dword ptr [rax]
    //         mov     rax, qword ptr [rbp - 16]
    //         mov     dword ptr [rax], ecx
    //         mov     ecx, dword ptr [rbp - 20]
    //         mov     rax, qword ptr [rbp - 8]
    //         mov     dword ptr [rax], ecx
    //         pop     rbp
//31//         ret
template <typename T>
requires (!std::integral<T>)
void swap(T& t, T& t2){
    auto tmp = std::move(t2);
    t2 = std::move(t);
    t = std::move(tmp);
}


// funny, but not optimal
// XOR version:
// void swap<int>(int&, int&):
//15//         push    rbp
    //         mov     rbp, rsp
    //         mov     qword ptr [rbp - 8], rdi
    //         mov     qword ptr [rbp - 16], rsi
    //         mov     rax, qword ptr [rbp - 16]
    //         mov     ecx, dword ptr [rax]
    //         mov     rax, qword ptr [rbp - 8]
    //         xor     ecx, dword ptr [rax]
    //         mov     dword ptr [rax], ecx
    //         mov     rax, qword ptr [rbp - 8]
    //         mov     ecx, dword ptr [rax]
    //         mov     rax, qword ptr [rbp - 16]
    //         xor     ecx, dword ptr [rax]
    //         mov     dword ptr [rax], ecx
    //         mov     rax, qword ptr [rbp - 16]
    //         mov     ecx, dword ptr [rax]
    //         mov     rax, qword ptr [rbp - 8]
    //         xor     ecx, dword ptr [rax]
    //         mov     dword ptr [rax], ecx
    //         pop     rbp
//36//         ret
template <std::integral T>
void swap(T& t, T& t2){
    t ^= t2;
    t2 ^= t;
    t ^= t2;
}

struct Base {
    int x;
    int y;
};

int main() {
    int x = 10;
    int y = 20;
    swap(x, y);
    assert(x == 20 && y == 10);

    Base b{20,30};
    Base b2{10,5};
    swap(b, b2);
    assert(b.x == 10 && b.y == 5 && b2.x == 20 && b2.y == 30);

    float ab = 0.1f;
    float bc = 0.2f;
    swap(ab, bc);
    assert(ab == 0.2f && bc == 0.1f);
}