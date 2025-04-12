#include <iostream>
#include <vector>
#include <array>
#include <utility>
#include <optional>

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

enum class Operation {PUSH, POP, DUP, SWAP, DROP, UNKNOWN, ADD, TOP};
constexpr Operation ops[] = {Operation::PUSH, Operation::POP, Operation::DUP, Operation::SWAP, Operation::DROP, Operation::ADD, Operation::TOP};
constexpr std::string_view mnemonics[] = {"PUSH", "POP", "DUP","SWAP", "DROP", "ADD", "TOP"};
constexpr size_t ins_c = std::size(mnemonics);


struct Instruction {
    Operation op;
    int data{};
};

struct StackVM {
    Stack<int> s;
    consteval std::optional<int> execute(Instruction ins) {
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
            case Operation::TOP: {
                return s.top();
            }
        }
        return {};
    }
};

consteval int token_to_index(const auto& v) {
    for (size_t i = 0; i < ins_c; ++i) {
        if (mnemonics[i] == v) {
            return i;
        }
    }
    return -1;
}

consteval bool isnum(char el) {
    constexpr std::array<char, 10> nums{'0','1','2','3','4','5','6','7','8','9'};
    for (auto n: nums){
        if (el == n){
            return true;
        }
    }
    return false;
}

consteval bool isalpha(char el) {
    constexpr std::array<char, 52> alphas{
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
        };
    for (auto a: alphas){
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
    auto index = token_to_index(token);
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
        case Operation::TOP:
            return Instruction{.op = op};
        case Operation::PUSH: {
            auto num = atoi(std::string_view{str.begin()+current});
            return Instruction{.op = op, .data = num};    
        }
    }
    return Instruction{.op = Operation::UNKNOWN};
}

consteval auto parse_to_ranges(std::string_view s) {
    std::vector<std::pair<int, int>> result;
    size_t head{};
    size_t tail{};
    bool searching = true;
    for (auto it = s.begin(), ite = s.end(); it != ite; ++it) {
        if (!isalpha(*it) && searching) {++head; ++tail; continue;}else{searching = false;}
        if (*it != ';') {
            ++tail;
            continue;
        }
        result.push_back({head, tail});
        if (std::next(it) != s.end()){
            ++tail;
            head = tail;
            searching = true;
        }
    }
    return result;
}

consteval auto execute(std::string_view s) {
    StackVM vm;
    auto ranges = parse_to_ranges(s);
    std::optional<int> res;
    for (auto [begin, end]: ranges)
        res = vm.execute(fetch({s.begin() + begin, s.begin() + end}));
    return res;
}

constexpr auto raw = R"(
        PUSH 10;
        PUSH 10;
        PUSH 5;
        PUSH 5;
        PUSH 10;
        PUSH 1;
        PUSH 1;
        ADD;
        ADD;
        ADD;
        ADD;
        ADD;
        ADD;
        PUSH -10;
        PUSH 10;
        SWAP;
        ADD;
        POP;
        TOP;
)";

int main() {
    constexpr auto res = execute(raw);
    static_assert(res.has_value());
    static_assert(res.value() == 42);
}