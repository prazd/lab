#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <cassert>
#include <optional>
#include <array>
#include <chrono>
#include <algorithm>

template <size_t sz, typename Func, typename ... Args>
struct WorkerPool {
private:
    struct Task {
      Func f;
      std::tuple<Args...> args;
      explicit Task(Func f, Args ... args): f(f), args(std::forward<Args>(args)...){}
    };
    struct TaskPool {
      mutable std::mutex mt;
      std::queue<Task> tasks{};
      bool empty() const {
        std::lock_guard lg(mt);
        return tasks.empty();
      }
      std::optional<Task> get_task(){
        std::lock_guard lg(mt);
        if (tasks.empty()) return std::nullopt;
        auto r = tasks.front(); tasks.pop();
        return r;
      }
      void add(Task t){
        std::lock_guard lg(mt);
        tasks.push(t);
      }
    } task_pool;

    struct alignas(64) ThreadStatus {
      std::atomic<bool> running{};
      std::atomic<bool> sleep{};
      std::atomic<bool> in_progress{};
    };

    std::array<ThreadStatus, sz> thread_status;
    std::vector<std::thread> pool{};

    static void t_func(ThreadStatus& ts, TaskPool& tp){
      while(ts.running.load()){
          if (!ts.sleep.load()){
            if (auto t = tp.get_task(); t.has_value()){
              ts.in_progress.store(true);
              auto v = t.value();
              std::apply(v.f, v.args);
              ts.in_progress.store(false);
            }
          } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
          }
      }    
    }
public:
    WorkerPool(){
      pool.reserve(sz);
      for (auto& el: thread_status) {el.running.store(true); el.sleep.store(true);}
      for (int i = 0; i < sz; ++i){
        pool.emplace_back(t_func, std::ref(thread_status[i]), std::ref(task_pool));
      }
    }
    void add(Func f, Args&& ... args){
      task_pool.add(Task{f, std::forward<Args>(args)...});
    }
    void run(){
      for (auto& el: thread_status) {el.sleep.store(false);}
      while(!task_pool.empty() || std::any_of(thread_status.begin(), thread_status.end(), [](auto& el){return el.in_progress.load();}))
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      for (auto& el: thread_status) {el.sleep.store(true);}
    }

    ~WorkerPool(){
      while(!task_pool.empty() || std::any_of(thread_status.begin(), thread_status.end(), [](auto& el){return el.in_progress.load();}))
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      for (auto& el: thread_status) {el.running.store(false); el.sleep.store(false);}
      for (auto&& t: pool) if (t.joinable()) t.join();
    }
};

int main(){
  std::array<int, 10> arr{};
  WorkerPool<4, void(*)(int&),int&> wp;
  wp.add([](int& x){x = 0;}, arr[0]);
  wp.add([](int& x){x = 1;}, arr[1]);
  wp.add([](int& x){x = 2;}, arr[2]);
  wp.add([](int& x){x = 3;}, arr[3]);
  wp.add([](int& x){x = 4;}, arr[4]);
  wp.add([](int& x){x = 5;}, arr[5]);
  wp.add([](int& x){x = 6;}, arr[6]);
  wp.add([](int& x){x = 7;}, arr[7]);
  wp.add([](int& x){x = 8;}, arr[8]);
  wp.add([](int& x){x = 9;}, arr[9]);
  wp.run();

  assert(arr[0] == 0);
  assert(arr[1] == 1);
  assert(arr[2] == 2);
  assert(arr[3] == 3);
  assert(arr[4] == 4);
  assert(arr[5] == 5);
  assert(arr[6] == 6);
  assert(arr[7] == 7);
  assert(arr[8] == 8);
  assert(arr[9] == 9);
}