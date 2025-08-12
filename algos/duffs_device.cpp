#include <iostream>
#include <numeric>
#include <cassert>

// Duff's Device: an optimized loop unrolling technique from the 1980s
template <typename T>
void duff_copy(T* to, const T* from, size_t count) {
    size_t n = (count + 7) / 8;
    switch(count % 8) {
        case 0: do {*to++ = *from++;
        case 7:     *to++ = *from++;
        case 6:     *to++ = *from++;
        case 5:     *to++ = *from++;
        case 4:     *to++ = *from++;
        case 3:     *to++ = *from++;
        case 2:     *to++ = *from++;
        case 1:     *to++ = *from++;
                } while(--n);
    }
}


int main() {
    int src[13]{};
    std::iota(std::begin(src), std::end(src), 0);
    int dst[13]{};
    duff_copy(dst, src, 13);
    assert(std::equal(std::begin(src), std::end(src), std::begin(dst)));
}