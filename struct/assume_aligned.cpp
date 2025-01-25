#include <iostream>

template <typename T>
void print(const T& c) {
    auto ptr = (unsigned char*)c;
    for(auto it = ptr + sizeof(c)-1, ite = ptr-1;it != ite;--it)
        std::cout << (void*)it << " " << (unsigned)(*it) << "\n";
}

void process_aligned_data(float* arr, size_t n) {
    auto* data = std::assume_aligned<32>(arr);
    for(size_t i = 0;i < n;++i) {
        data[i]*=2.f;
    }
}

int main() {
    alignas(32) float data[3]{0.1f, 0.2f, 0.3f};
    process_aligned_data(data, 3);
    print(data);
}