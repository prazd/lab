#include <iostream>

struct TCPClient {
    TCPClient(std::string ip, unsigned short port, int timeout){
        std::cout << "Client:" <<"\n";
        std::cout << "IP:" << ip << "\n";
        std::cout << "Port:" << port << "\n";
        std::cout << "Timeout:" << timeout << "\n";
    }
};

struct ClientBuilder {
    std::string ip{};
    unsigned short port{};
    int timeout{};
    ClientBuilder() = default;
    
    ClientBuilder& setIp(std::string_view value) {
        ip = value;
        return *this;
    }

    ClientBuilder& setPort(unsigned short p) {
        port = p;
        return *this;
    }

    ClientBuilder& setTimeout(int t) {
        timeout = t;
        return *this;
    }

    TCPClient build() {
        if (ip.empty() || !port || !timeout)
            throw std::invalid_argument("Build error");
        return TCPClient(ip, port, timeout);
    }
};


int main() {
    auto client = ClientBuilder()
                  .setIp("localhost")
                  .setPort(8080)
                  .setTimeout(100)
                  .build();
}