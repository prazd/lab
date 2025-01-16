#include <iostream>
#include <array>

template <typename T, size_t cap>
class CircularBuffer {
    std::array<T, cap> buffer{};
    size_t head{};
    size_t tail{};
    bool full = false;
public:
    CircularBuffer() = default;

    void push(const T& val) {
        buffer[tail] = val;
        tail = (tail + 1) % cap;
        if (full) {
            head = (head + 1) % cap;
        }
        full = head == tail;
    }

    T pop() {
        if (empty()){
            throw std::runtime_error("Error");
        }
        auto val = buffer[head];
        head = (head + 1) % cap;
        full = false;
        return val;
    }

    bool empty() const {
        return !full && (head == tail);
    }
};

int main(){
    CircularBuffer<int, 3> cb;
    cb.push(10);
    cb.push(20);
    cb.push(30);
    cb.push(40);
    cb.push(50);
    while(!cb.empty())
        std::cout << cb.pop() << "\n";
}