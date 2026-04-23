#include <iostream>
#include <chrono>
#include <algorithm>
#include <cassert>

constexpr size_t SZ = 3'000'000;
int ARR1[SZ + 1];
int ARR2[SZ + 1];

using ARR_REF = const int(&)[SZ + 1];

void __attribute__((constructor)) init_arrays() {
    std::generate(std::begin(ARR1), std::end(ARR1), []{return rand() % 10;});
    ARR1[SZ] = 10;
    std::copy(std::begin(ARR1), std::end(ARR1), std::begin(ARR2));
    assert(std::equal(std::begin(ARR1), std::end(ARR1), std::begin(ARR2)));
}

size_t computed_goto(ARR_REF arr){
    size_t i = 0;
    size_t result{};
    void* handlers[] = {&&L1, &&L2, &&L3, &&L4, &&L5, &&L6, &&L7, &&L8, &&L9, &&L10, &&END};
    goto *handlers[arr[i++]];
L1:
    result += 2;
    goto *handlers[arr[i++]];
L2:
    result += 4;
    goto *handlers[arr[i++]];
L3:
    result += 6;
    goto *handlers[arr[i++]];
L4:
    result += 8;
    goto *handlers[arr[i++]];
L5:
    result += 10;
    goto *handlers[arr[i++]];
L6:
    result += 12;
    goto *handlers[arr[i++]];
L7:
    result += 14;
    goto *handlers[arr[i++]];    
L8:
    result += 16;
    goto *handlers[arr[i++]];
L9:
    result += 18;
    goto *handlers[arr[i++]];
L10:
    result += 20;
    goto *handlers[arr[i++]];
END:
    return result;
}

size_t switch_version(ARR_REF arr) {
    size_t result{};
    for (size_t i = 0, e = std::size(arr); i < e; ++i){
        switch(arr[i]){
            case 0: {result += 2;        break;}
            case 1: {result += 4;        break;}
            case 2: {result += 6;        break;}
            case 3: {result += 8;        break;}
            case 4: {result += 10;       break;}
            case 5: {result += 12;       break;}
            case 6: {result += 14;       break;}
            case 7: {result += 16;       break;}
            case 8: {result += 18;       break;}
            case 9: {result += 20;       break;}
            case 10:{return result;}
        }
    }
    return 0;
}

auto bench(const char* str, size_t(*f)(ARR_REF), ARR_REF arr) {
    auto begin = std::chrono::steady_clock::now();
    auto r = f(arr);
    auto end = std::chrono::steady_clock::now();
    std::cout << str << " res:" << r <<  " time:"  << std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count() << "ms\n";
    return r;
}

int main(){
    auto r1 = bench("Switch", switch_version, ARR1);
    auto r2 = bench("Computed goto", computed_goto, ARR2);
    assert(r1 == r2);
}