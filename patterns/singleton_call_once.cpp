#include <iostream>
#include <mutex>
#include <memory>

class Singleton{
    static inline std::unique_ptr<Singleton> instance = nullptr;
    static inline std::once_flag flag;
    int data;
    Singleton(): data(100){}
    Singleton(const Singleton&) = delete;
public:
    static Singleton& get_instance() {
        std::call_once(flag, [](){
            instance = std::unique_ptr<Singleton>(new Singleton());
        });
        return *instance;
    }
    ~Singleton() = default;
};


int main() {
    auto& instance = Singleton::get_instance();
    auto& instance2 = Singleton::get_instance();
}