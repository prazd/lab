#include <iostream>
#include <stack>
#include <sstream>
#include <vector>
#include <array>

#define DEBUG 0

template <typename T>
class Stack {
    size_t cap;
    size_t sz;
    T* ptr;
public:    
    consteval Stack(size_t cap): cap(cap), sz(0), ptr(new T[cap]{}){}
    consteval Stack(): cap(4), sz(0), ptr(new T[cap]{}) {}
    consteval void push(T val) {
        if (cap == sz) {
            cap*=2;
            T* nptr = new T[cap];
            std::copy(ptr, ptr + sz, nptr);
            delete[] ptr;
            ptr = nptr;
        }
        ptr[sz++] = val;
    }
    consteval void pop() {
        if (!sz) throw std::underflow_error("pop error");
        --sz;
    }
    consteval bool empty() const {
        return sz == 0;
    }
    consteval T top() const {
        if (!sz) throw std::underflow_error("top error");
        return ptr[sz-1];
    }
    constexpr ~Stack() {delete[] ptr;}
};

enum class Operation {PUSH, POP, DUP, SWAP, DROP, UNKNOWN, ADD};
constexpr Operation ops[] = {Operation::PUSH, Operation::POP, Operation::DUP, Operation::SWAP, Operation::DROP, Operation::ADD};
constexpr std::string_view mnemonics[] = {"PUSH", "POP", "DUP","SWAP", "DROP", "ADD"};
constexpr size_t ins_c = std::size(mnemonics);


struct Instruction {
    Operation op;
    int data{};
};

struct StackVM {
    Stack<int> s;
    consteval long long execute(Instruction ins) {
        switch (ins.op) {
            case Operation::POP:    
            case Operation::DROP:    
                {if (!s.empty()) s.pop(); break;}
            case Operation::PUSH:    {s.push(ins.data); break;}
            case Operation::DUP:     {s.push(s.top()); break;}
            case Operation::SWAP:    {if (s.empty()) break; auto t = s.top(); s.pop(); if (s.empty()) break; auto d = s.top(); s.pop(); s.push(t); s.push(d); break;}
            case Operation::UNKNOWN: {break;}
            case Operation::ADD: {
                if (s.empty()) break;
                auto r = s.top(); s.pop();
                if (s.empty()) break;
                auto l = s.top(); s.pop();
                s.push(r + l);
                break;
            }
        }
        return s.top();
    }
};

consteval int get_index(const auto& v) {
    for (size_t i = 0; i < ins_c; ++i) {
        if (mnemonics[i] == v) {
            return i;
        }
    }
    return -1;
}

consteval bool isnum(char el) {
    constexpr std::array<char, 10> nums{'0','1','2','3','4','5','6','7','8','9'};
    for (auto a: nums){
        if (el == a){
            return true;
        }
    }
    return false;
}

consteval int atoi(std::string_view s) {
    int res{};
    int tmp{};
    bool gz = true;
    bool update = false;
    auto imax = std::numeric_limits<int>::max();
    auto imin = std::numeric_limits<int>::min();
    auto ret_func = [&gz, &res]{return gz ? res : res * -1;};
    for (auto el: s) {
        if (el == '-') {if (update) return ret_func(); update = true; gz = false; continue;};
        if (!isnum(el)) return ret_func();
        tmp = el - '0';
        if (!res) {
            res = tmp;
            update = true;
            continue;
        }
        if (imax / 10 < res || ((imax / 10 == res && tmp > 7)))
            return gz ? imax : imin;
        
        res *= 10;
        res += tmp;
        update = true;
    }
    return ret_func();
}

consteval Instruction fetch(std::string_view str) {
    std::string token;
    size_t current = 0;
    for (auto el: str) {
        ++current;
        if (el == ' ') break;
        token += el;
    }
    auto index = get_index(token);
    if (index == -1) {
        return Instruction{.op = Operation::UNKNOWN};
    }
    auto op = ops[index];
    switch (op) {
        case Operation::POP:
        case Operation::DUP:
        case Operation::DROP:
        case Operation::UNKNOWN:
        case Operation::ADD:
        case Operation::SWAP:
            return Instruction{.op = op};
        case Operation::PUSH: {
            
            auto num = atoi(std::string_view{str.begin()+current});
            return Instruction{.op = op, .data = num};
        }
    }
    return Instruction{.op = Operation::UNKNOWN};
}

consteval auto execute(std::string_view s) {
    StackVM vm;
    return vm.execute(fetch(s));
}

consteval auto mexecute(std::string_view s) {
    StackVM vm;
    size_t head{};
    size_t tail{};
    int res{};
    for (auto it = s.begin(), ite = s.end(); it != ite; ++it) {
        if (*it == ';') {
            #if DEBUG
            std::cout << std::string_view{s.begin() + head, s.begin() + tail} << '\n';
            #endif
            res = vm.execute(fetch({s.begin() + head, s.begin() + tail}));
            if (std::next(it) != s.end()){
                ++tail;
                head = tail;
            }
        } else {
            ++tail;
        }
    }
    return res;
}

constexpr auto raw = "PUSH 1;DUP;PUSH 10;PUSH 30;ADD;ADD;ADD;PUSH -10;PUSH 10;ADD;POP;";

int main() {
    constexpr auto res = mexecute(raw);
    static_assert(res == 42);
}