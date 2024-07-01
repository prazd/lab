#include <iostream>
#include <thread>
template<typename T>
concept UnsignedNumeric = std::is_arithmetic_v<T>;

template <UnsignedNumeric T>
void byte_visualizer() {
    size_t counter = std::numeric_limits<T>::max();
    T a = 0;
    unsigned char* res = reinterpret_cast<unsigned char*>(&a);
    size_t sza = sizeof(a);
    for (size_t i = 0;i<counter;++i) {
        std::cout << "A value:" << a << std::endl;
        std::cout << "Bytes value:" << std::endl;
        for (size_t j = 0;j<sza;++j) {
            std::cout << static_cast<unsigned int>(res[j]) << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::cout << "\x1B[2J\x1B[H";
        ++a;
    }
}
  
int main()
{  
    byte_visualizer<unsigned char>();
}