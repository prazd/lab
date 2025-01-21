#include <iostream>
#include <sstream>


unsigned ipv4stoi(const std::string& ip) {
    std::stringstream ss(ip);
    std::string tmp;
    unsigned res = 0;
    char step = 0;
    while(std::getline(ss, tmp, '.')){
        unsigned r = std::atoi(tmp.data());
        res |= r << step;
        step += CHAR_BIT;
    }
    return res;
}

std::string ipv4itoa(unsigned ip) {
    std::string result;
    unsigned char tmp = 0;
    while(ip){
        tmp = ip;
        result += std::to_string(tmp) + ".";
        ip>>=CHAR_BIT;
    }
    result.pop_back();
    return result;
}

template <typename T>
void printb(T& v) {
    auto ptr = reinterpret_cast<unsigned char*>(&v);
    for(auto it = ptr + sizeof(v) - 1, ite = ptr-1; it != ite; --it)
        std::cout << (void*)it << " " << (unsigned)(*it) << "\n";
}

int main(){
    auto ip = ipv4stoi("192.168.0.1");
    std::cout << "IP 192.168.0.1" << "(" << ip << "):\n";
    printb(ip);
    std::cout << "From " << ip << ":\n";
    std::cout << ipv4itoa(ip) << "\n";
}
