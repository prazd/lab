#include <iostream>
#include <thread>

class ThreadRaII{
public:
    enum class Dtor{join, detach};
    ThreadRaII(std::thread&& t, Dtor action): action(action), t(std::move(t)){}
    ~ThreadRaII() {
        if (t.joinable()) {
            if (action == Dtor::join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }
    ThreadRaII(ThreadRaII&&) = default;
    ThreadRaII& operator=(ThreadRaII&&) = default;
    std::thread& get() {
        return t;
    }
private:
    Dtor action;
    std::thread t;
};

int main() {
}