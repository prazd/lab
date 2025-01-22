#include <thread>
#include <iostream>
#include <barrier>
#include <mutex>

std::mutex printmt;
template <typename... Args>
void print(Args&&... args) {
    std::lock_guard lg(printmt);
    (std::cout << ... << args) <<  "\n";
}

std::barrier sync_point(3, [](){print("Barrier done");});

void work(int id) {
    print("Before work", id);
    sync_point.arrive_and_wait();
    print("After work", id); 
}


int main(){
   std::vector<std::thread> threads;
   for(int i = 0; i < 3; ++i) {
       threads.emplace_back(work, i);
   }
   for(auto& el: threads)
        el.join();
}
