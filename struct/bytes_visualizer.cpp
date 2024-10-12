#include <iostream>
#include <thread>
#include <bitset>
#include <type_traits>

template<typename T>
concept UnsignedNumeric = std::is_arithmetic_v<T>;

template<typename T>
concept SignedNumeric = std::is_signed_v<T> && std::is_arithmetic_v<T>;

template<typename T>
concept ValueType = SignedNumeric<T> || UnsignedNumeric<T>;


template <ValueType T>
void byte_visualizer(long long msec) {
    size_t counter = std::numeric_limits<T>::max();
    T a = 0;
    T b = 0;
    unsigned char* res = reinterpret_cast<unsigned char*>(&a);
    unsigned char* res_b = reinterpret_cast<unsigned char*>(&b);
    size_t sza = sizeof(a);
    std::cout << "\x1B[2J\x1B[H";
    for (size_t i = 0; i <= counter; ++i) {
        std::cout << "\x1B[H";
        std::cout << "A address:" <<  static_cast<void*>(&a) << "\n";
        std::cout << "A value:" << (long)a << "\n";
        std::cout << "Bytes value:" << "\n";
        for (int j = (sza-1); j >= 0; --j) {
            auto value = static_cast<unsigned int>(res[j]);
            std::bitset<8> bits(value);
            std::cout << "res["<<j<<"]:" <<static_cast<void*>(res+j) <<"    value: " << value << "     bit_value: "<< bits<<"\n";
        }
        ++a;
        if constexpr(SignedNumeric<T>) {
            std::cout << "\n";
            std::cout << "B address:" <<  static_cast<void*>(&b) << "\n";
            std::cout << "B value:" << (long)b << "\n";
            std::cout << "Bytes value:" << "\n";
            for (int j = (sza-1); j >= 0; --j) {
                auto value = static_cast<unsigned int>(res_b[j]);
                std::bitset<8> bits(value);
                std::cout << "res_b["<<j<<"]:" <<static_cast<void*>(res_b+j) <<"    value: " << value << "     bit_value: "<< bits << "\n";
            }
            --b;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    }
}
 
int main()
{
    std::cout << "Select version:" << "\n";
    std::cout << "1: u8"<< "\n";
    std::cout << "2: u16" << "\n";
    std::cout << "3: u32"<<"\n";
    std::cout << "4: u64"<<"\n";
    std::cout << "5: i8"<<"\n";
    std::cout << "6: i16"<<"\n";
    std::cout << "7: i32"<<"\n";
    std::cout << "8: i64"<<"\n";
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
        case 5:
            func = &byte_visualizer<char>;
            break;
        case 6:
            func = &byte_visualizer<short>;
            break;
        case 7:
            func = &byte_visualizer<int>;
            break; 
        case 8:
            func = &byte_visualizer<long>;
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