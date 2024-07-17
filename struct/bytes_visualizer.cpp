#include <iostream>
#include <thread>
#include <bitset>

template<typename T>
concept UnsignedNumeric = std::is_arithmetic_v<T>;

template<typename T>
concept SignedNumeric = std::is_signed_v<T> && std::is_arithmetic_v<T>;


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
        for (int j = (sza-1); j >= 0; --j) {
            auto value = static_cast<unsigned int>(res[j]);
            std::bitset<8> bits(value);
            std::cout << "res["<<j<<"]:" <<static_cast<void*>(res+j) <<"    value: " << value << "     bit_value: "<< bits<<std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
        std::cout << "\x1B[2J\x1B[H";
        ++a;
    }
}

template <SignedNumeric T>
void byte_visualizer_signed(long long msec) {
    size_t counter = std::numeric_limits<T>::max();
    T a = 0;
    T b = 0;
    unsigned char* res_a = reinterpret_cast<unsigned char*>(&a);
    unsigned char* res_b = reinterpret_cast<unsigned char*>(&b);
    size_t sza = sizeof(a);
    for (size_t i = 0; i <= counter; ++i) {
        std::cout << "A address:" <<  static_cast<void*>(&a) << std::endl;
        std::cout << "A value:" << a << std::endl;
        std::cout << "Bytes value:" << std::endl;
        for (int j = (sza-1); j >= 0; --j) {
            auto value = static_cast<unsigned int>(res_a[j]);
            std::bitset<8> bits(value);
            std::cout << "res_a["<<j<<"]:" <<static_cast<void*>(res_a+j) <<"    value: " << value << "     bit_value: "<< bits<<std::endl;
        }
        std::cout << std::endl;
        std::cout << "B address:" <<  static_cast<void*>(&b) << std::endl;
        std::cout << "B value:" << b << std::endl;
        std::cout << "Bytes value:" << std::endl;
        for (int j = (sza-1); j >= 0; --j) {
            auto value = static_cast<unsigned int>(res_b[j]);
            std::bitset<8> bits(value);
            std::cout << "res_b["<<j<<"]:" <<static_cast<void*>(res_b+j) <<"    value: " << value << "     bit_value: "<< bits<<std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
        std::cout << "\x1B[2J\x1B[H";
        ++a;
        --b;
    }
}


  
int main()
{
    std::cout << "Select version:"<<std::endl;
    std::cout << "1: u8"<<std::endl;
    std::cout << "2: u16"<<std::endl;
    std::cout << "3: u32"<<std::endl;
    std::cout << "4: u64"<<std::endl;
    std::cout << "5: i8"<<std::endl;
    std::cout << "6: i16"<<std::endl;
    std::cout << "7: i32"<<std::endl;
    std::cout << "8: i64"<<std::endl;
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
            func = &byte_visualizer_signed<char>;
            break;
        case 6:
            func = &byte_visualizer_signed<short>;
            break;
        case 7:
            func = &byte_visualizer_signed<int>;
            break; 
        case 8:
            func = &byte_visualizer_signed<long>;
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