#include <iostream>
#include <cassert>
#include <thread>

constexpr int MAX_THREADS = 16;

template <typename T>
struct Node {
    T val;
    Node<T>* next;
    Node(): val(0), next(nullptr){}
    Node(T val): val(val), next(nullptr){}
};

std::atomic<Node<int>*> hazard_pointers[MAX_THREADS];

template <typename T>
struct Stack {
    std::atomic<Node<T>*> head;
    Stack(): head(nullptr) {}
    Stack(T val){
        head.store(new Node<T>(val));
    };
    void push(T v) {
        auto next = new Node<T>{v};
        for (;;) {
            auto tmp = head.load();
            next->next = tmp;
            if (head.compare_exchange_strong(tmp, next)){
                return;
            }
        }
    }
    T pop(int thread_id) {
        for(;;){
            auto tmp = head.load();
            if (!tmp) {
                throw std::length_error("empty stack");
            }

            hazard_pointers[thread_id].store(tmp, std::memory_order_release);
            if (head.load(std::memory_order_acquire) != tmp) {
                continue;
            }
            
            auto n = tmp->next;
            if (head.compare_exchange_strong(tmp, n)) {
                hazard_pointers[thread_id].store(nullptr, std::memory_order_release);
                auto val = tmp->val;
                if (!is_hazard(tmp)) {
                    delete tmp;
                } else {
                    retire_node(tmp);
                }
                return val;
            }
        }
    }

    bool is_hazard(Node<T>* ptr) {
        for (int i = 0; i < MAX_THREADS; ++i) {
            if (hazard_pointers[i].load() == ptr) {
                return true;
            }
        }
        return false;
    }    

    void retire_node(Node<T>* ptr) {
        static thread_local std::vector<Node<T>*> retired_nodes;
        retired_nodes.push_back(ptr);
        if (retired_nodes.size() > 10) {
            for (auto it = retired_nodes.begin(); it != retired_nodes.end();) {
                if (!is_hazard(*it)) {
                    delete *it;
                    it = retired_nodes.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
        
    ~Stack() {
        auto h = head.load();
        while(h) {
            auto tmp = h->next;
            delete h;
            h = tmp;
        }
    }
};


int main() {
    Stack s{10};
    std::thread t1([&](){
        for (auto i = 1; i < 10; ++i) {
            s.push(i);
        }
    });
    std::thread t2([&](){
        for (auto i = 10; i < 20; ++i) {
            s.push(i);
        }
    });
    t1.join();
    t2.join();

    try {
        while(true) {
            std::cout << s.pop(0) << "\n";
        }
    } catch(std::length_error err) {
        std::cout << "Length err";
    }
}