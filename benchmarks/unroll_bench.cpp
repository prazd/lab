#include <iostream>
#include <algorithm>
#include <chrono>

#define REPEAT2(x,arr,n) x += arr[i + n];x += arr[i+n+1];
#define REPEAT4(x,arr,n)   REPEAT2(x,arr,n) REPEAT2(x,arr,n+2) 
#define REPEAT8(x,arr,n)   REPEAT4(x,arr,n) REPEAT4(x,arr,n+4)
#define REPEAT16(x,arr,n)  REPEAT8(x,arr,n) REPEAT8(x,arr,n+8) 
#define REPEAT32(x,arr,n)  REPEAT16(x,arr,n) REPEAT16(x,arr,n+16)
#define REPEAT64(x,arr,n)  REPEAT32(x,arr,n) REPEAT32(x,arr,n+32)
#define REPEAT128(x,arr,n) REPEAT64(x,arr,n) REPEAT64(x,arr,n+64)
#define REPEAT256(x,arr,n) REPEAT128(x,arr,n) REPEAT128(x,arr,n+128)
#define REPEAT512(x,arr,n) REPEAT256(x,arr,n) REPEAT256(x,arr,n+256)
#define REPEAT1024(x,arr,n) REPEAT512(x,arr,n) REPEAT512(x,arr,n+512)

constexpr size_t SZ = 1 << 28;
unsigned char A1[SZ];
unsigned char A2[SZ];
unsigned char A3[SZ];
unsigned char A4[SZ];
unsigned char A5[SZ];
unsigned char A6[SZ];
unsigned char A7[SZ];

void __attribute__((constructor)) init(){
    std::generate(A1, A1 + SZ, []{return (unsigned char)(rand() & 0xff);});
    std::copy(A1, A1 + SZ, A2);
    std::copy(A1, A1 + SZ, A3);
    std::copy(A1, A1 + SZ, A4);
    std::copy(A1, A1 + SZ, A5);
    std::copy(A1, A1 + SZ, A6);
    std::copy(A1, A1 + SZ, A7);
}

template <size_t n>
size_t rep(unsigned char(&arr)[SZ]){
    size_t res = 0;
    for (size_t i = 0; i < SZ; i+=n){
        if constexpr (n == 16){
            REPEAT16(res, arr,0)
        } else if (n == 32) {
            REPEAT32(res, arr,0)
        }  else if (n == 64) {
            REPEAT64(res, arr,0)
        } else if (n == 128) {
            REPEAT128(res, arr,0)
        } else if (n == 256){
            REPEAT256(res, arr,0)
        } else if (n == 512) {
            REPEAT512(res, arr,0)
        } else if (n == 1024) {
            REPEAT1024(res, arr,0)
        }
    }
    return res;
}

template <size_t n>
void bench(const char* text, unsigned char(&arr)[SZ]){
    auto begin = std::chrono::steady_clock::now();
    auto s = rep<n>(arr);
    auto end = std::chrono::steady_clock::now();
    std::cout << text << '(' << s << ')' << ':' << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << "ms\n";
}

int main(){
    bench<16>("REP16", A1);
    bench<32>("REP32", A2);
    bench<64>("REP64", A3);
    bench<128>("REP128", A4);
    bench<256>("REP256", A5);
    bench<512>("REP512", A6);
    bench<1024>("REP1024", A7);
}