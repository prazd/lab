#ifndef NES_SIMPLE_H
#define NES_SIMPLE_H
#include <iostream>
#include <optional>
#include <thread>
#include <queue>
#include <algorithm>

template <typename T, size_t sz>
struct SimpleDeque {
    int current_sz = 0;
    alignas(T) unsigned char buffer[sz * sizeof(T)]{};
    SimpleDeque() = default;

    struct Iter {
        unsigned char* current;
        Iter& operator++(){
            current = current = reinterpret_cast<unsigned char*>(reinterpret_cast<T*>(current) + 1);
            return *this;
        }
        Iter& operator--(){
            current = current = reinterpret_cast<unsigned char*>(reinterpret_cast<T*>(current) + 1);
            return *this;
        }
        Iter operator+(int i){
            return Iter{reinterpret_cast<unsigned char*>(reinterpret_cast<T*>(current) + i)};
        }
        T& operator*(){
            return *reinterpret_cast<T*>(current);
        }
        unsigned char* data(){
            return current;
        }
        bool operator!=(const Iter& iter){
            return iter.current != current;
        }
    };

    auto begin(){
        return Iter{buffer};
    }

    auto end(){
        return Iter{buffer + sizeof(T) * current_sz};
    }

    void erase(Iter begin, Iter end){
        current_sz = (begin.data() - buffer)/sizeof(T);
        for (; begin != end; ++begin)
            reinterpret_cast<T*>(begin.data())->~T();
    }

    template <typename U>
    void push_back(U&& value){
        new (buffer + current_sz++ * sizeof(T)) T(std::forward<U>(value));
    }

    template <typename U>
    void push_front(U&& value){
        push_back(std::forward<U>(value));
        for (int i = current_sz-2; i >= 0; --i)
            std::swap(this->operator[](i), this->operator[](i+1));
    }

    void pop_back(){
        if (current_sz) {
            reinterpret_cast<T*>(buffer + (current_sz-1) * sizeof(T))->~T();
            --current_sz;
        }
    }

    void pop_front(){
        for (int i = 0; i <= current_sz-2; ++i)
            std::swap(this->operator[](i), this->operator[](i+1));
        pop_back();
    }

    T& operator[](size_t index){
        return  *reinterpret_cast<T*>(buffer + index * sizeof(T));
    }
    T& back() {
        return  *reinterpret_cast<T*>(buffer + (current_sz-1) * sizeof(T));
    }
    const T& operator[](size_t index)const{
        return  *reinterpret_cast<const T*>(buffer + index * sizeof(T));
    }
    size_t size() const {
        return current_sz;
    }
    bool empty() const {
        return current_sz == 0;
    }

    void reset(){
        for (int i = 0; i < current_sz; ++i) reinterpret_cast<T*>(buffer + i * sizeof(T))->~T();
#if DEBUG_MODE
        std::memset(buffer, 0, sizeof(T) * sz);
#endif
        current_sz = 0;
    }
    ~SimpleDeque(){
        for (int i = 0; i < current_sz; ++i)  reinterpret_cast<T*>(buffer + i * sizeof(T))->~T();
    }
};


template <typename T, size_t sz>
struct SimpleDequeTC {
    std::array<T, sz> buffer;
    size_t current_sz = 0;

    auto begin(){
        return buffer.begin();
    }
    auto end(){
        return buffer.begin() + current_sz;
    }

    template <typename U>
    void push_back(U&& value){
        buffer[current_sz++] = value;
    }

    template <typename U>
    void push_front(U&& value){
        push_back(std::forward<U>(value));
        for (int i = current_sz-2; i >= 0; --i)
            std::swap(buffer[i], buffer[i+1]);
    }
    size_t size() const {
        return current_sz;
    }
    bool empty() const {
        return current_sz == 0;
    }

    void pop_back(){
        if (current_sz) --current_sz;
    }

    void pop_front(){
        for (int i = 0; i <= current_sz-2; ++i)
            std::swap(this->operator[](i), this->operator[](i+1));
        pop_back();
    }

    T& operator[](size_t index){
        return buffer[index];
    }
    T& back() {
        return buffer[current_sz-1];
    }

    void reset(){
        current_sz = 0;
    }
};

template <typename T, size_t sz>
struct CircleStack {
    alignas(T) unsigned char arr[sizeof(T) * sz]{};
    size_t current = 0;
    unsigned char* ptr = arr;
    void els_copy(const CircleStack& rhs){
        ptr += rhs.ptr - rhs.arr;
        size_t current_tmp = rhs.current;
        auto ptr_rhs = rhs.ptr;
        auto ptr_cur = ptr;
        while(current_tmp){
            ptr_cur = ptr_cur + ((ptr_cur != arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1)));
            ptr_rhs = ptr_rhs + ((ptr_rhs != rhs.arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1)));
            new (ptr_cur) T(*std::launder(reinterpret_cast<T*>(ptr_rhs)));
            --current_tmp;
        }
        current = rhs.current;
    }
    void els_move(CircleStack&& rhs){
        ptr += rhs.ptr - rhs.arr;
        size_t current_tmp = rhs.current;
        auto ptr_rhs = rhs.ptr;
        auto ptr_cur = ptr;
        while(current_tmp){
            ptr_cur = ptr_cur + ((ptr_cur != arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1)));
            ptr_rhs = ptr_rhs + ((ptr_rhs != rhs.arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1)));
            new (ptr_cur) T(std::move(
                    *std::launder(reinterpret_cast<T*>(ptr_rhs))));
            --current_tmp;
            std::launder(reinterpret_cast<T*>(ptr_rhs))->~T();
        }
        current = rhs.current;
        rhs.current = 0;
        rhs.ptr = rhs.arr;
    }

    CircleStack() = default;
    CircleStack(const CircleStack& rhs){
        els_copy(rhs);
    }
    CircleStack(CircleStack&& rhs){
        els_move(std::move(rhs));
    }

    CircleStack& operator=(const CircleStack& rhs){
        if (&rhs != this) {
            while(!empty()) pop();
            ptr = arr;
            els_copy(rhs);
        }
        return *this;
    }

    CircleStack& operator=(CircleStack&& rhs){
        if (&rhs != this) {
            while(!empty()) pop();
            ptr = arr;
            els_move(std::move(rhs));
        }
        return *this;
    }

    template <typename U>
    void push(U&& value){
        if (current == sz) std::launder(reinterpret_cast<T*>(ptr))->~T();
        new (ptr) T(std::forward<U>(value));
        current = std::min(current + 1, sz);
        ptr += (ptr + sizeof(T) == arr + sz * sizeof(T)) ? ptrdiff_t(-1 * (sizeof(T) * (sz-1))) : ptrdiff_t(sizeof(T));
    }

    void pop(){
        current -= ((current - 1) < current) * 1;
        ptr += (ptr != arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1));
        std::launder(reinterpret_cast<T*>(ptr))->~T();
    }

    bool empty() const {
        return !current;
    }

    T& top() {
        return *std::launder(reinterpret_cast<T*>(ptr + ((ptr != arr) ? ptrdiff_t(-1 * sizeof(T)) : ptrdiff_t(sizeof(T) * (sz-1)))));
    }
    ~CircleStack(){
        while(!empty()) pop();
    }
};

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
    }
    void run_and_wait(){
        for (auto& el: thread_status) {el.sleep.store(false);}
        while(!task_pool.empty() || std::any_of(thread_status.begin(), thread_status.end(), [](auto& el){return el.in_progress.load();}));
        for (auto& el: thread_status) {el.sleep.store(true);}
    }

    ~WorkerPool(){
        while(!task_pool.empty() || std::any_of(thread_status.begin(), thread_status.end(), [](auto& el){return el.in_progress.load();}));
        for (auto& el: thread_status) {el.running.store(false); el.sleep.store(false);}
        for (auto&& t: pool) if (t.joinable()) t.join();
    }
};
#endif