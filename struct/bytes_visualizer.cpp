#include <iostream>
#include <thread>
template<typename T>
concept UnsignedNumeric = std::is_arithmetic_v<T>;

template <UnsignedNumeric T>
void byte_visualizer(long long msec) {
    size_t counter = std::numeric_limits<T>::max();
    T a = 0;
    unsigned char* res = reinterpret_cast<unsigned char*>(&a);
    size_t sza = sizeof(a);
    for (size_t i = 0; i <= counter; ++i) {
        std::cout << "A address:" <<  static_cast<void*>(&a) << std::endl;
        std::cout << "A value:" << a << std::endl;
        std::cout << "Bytes value:" << std::endl;
        for (size_t j = 0;j<sza;++j) {
            std::cout << "res["<<j<<"]:" <<static_cast<void*>(res+j) <<" value:    " <<static_cast<unsigned int>(res[j]) << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
        std::cout << "\x1B[2J\x1B[H";
        ++a;
    }
}
  
int main()
{
    std::cout << "Select version:"<<std::endl;
    std::cout << "1: u8"<<std::endl;
    std::cout << "2: u16"<<std::endl;
    std::cout << "3: u32"<<std::endl;
    std::cout << "4: u64"<<std::endl;
    unsigned short a;
    std::cout <<">> ";
    std::cin >> a;
    void (*func)(long long) = nullptr;
    switch (a) {
        case 1:
            func = &byte_visualizer<unsigned char>;
            break;
        case 2:
            func = &byte_visualizer<unsigned short>;
            break;
        case 3:
            func = &byte_visualizer<unsigned int>;
            break;
        case 4:
            func = &byte_visualizer<unsigned long>;
            break;
        default:
            std::cout  << "Unsupported command";
            return 1;
    }
    std::cout << "Timeout value:" << std::endl;
    std::cout << ">> ";
    long long msec;
    std::cin >> msec;

    func(msec);
}