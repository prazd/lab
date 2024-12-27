#include <iostream>
#include <thread>
#include <chrono>
#include <syncstream>


// linux / g++
#define sync_cout std::osyncstream(std::cout)

template <typename Callback, typename Duration>
struct Timer {
    Timer(Callback cb,const Duration interval) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(interval);
        sync_cout << "Timer: Starting interval of" << ms << "\n";
        t = std::jthread([&](std::stop_token stop_token){
            while(!stop_token.stop_requested()) {
                sync_cout << "Timer: Running callback:" << val.load() << "\n";
                ++val;
                cb();
                sync_cout << "Timer: Sleeping...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            }
            sync_cout << "Timer: Exit\n" << "\n";
        });
    }
    void stop() {
        t.request_stop();
    }
private:
    std::jthread t;
    std::atomic_int32_t val{0};
};

using namespace std::chrono_literals;

int main() {
    Timer cr([](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        sync_cout << "Done work" << "\n";
    }, 1s);
    std::this_thread::sleep_for(10s);
    sync_cout << "Main stop" << "\n";
    cr.stop();
}