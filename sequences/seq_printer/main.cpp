#include <iostream>
#include <gmpxx.h>

template <typename CondFunc, 
          typename NextFunc,
          typename PrintCond = decltype([](mpz_class& n){return true;}),
          typename State = mpz_class>
struct SeqPrinter {
    [[no_unique_address]] CondFunc  cf;
    [[no_unique_address]] NextFunc  nf;
    [[no_unique_address]] PrintCond pc;
    void operator()(State n){
        std::cout << "Starting sequence with: " << n << "\n";
        while(cf(n)){
            nf(n);
            if (pc(n)) {
               std::cout << n << " ";
            }
        }
        std::cout << "\nSequence complete.\n";
    }
};

// Default state examples
void print_collatz_seq(mpz_class start) {
    SeqPrinter<decltype([](auto& n){return n != 1;}),
    decltype([](auto& n){
    if (n % 2 == 0) {
        n = n / 2;
    } else {
        n = n * 3;
        n = n + 1;
    }
    })>()(start);
}

mpz_class reverse_num(mpz_class num) {
    mpz_class reverse = 0;
    mpz_class original = num;
    while(num) {
        reverse = (reverse * 10) + (num % 10);
        num /= 10;
    }
    return reverse;
}

bool is_num_palindrom(mpz_class num) {
    return reverse_num(num) == num;
}

void print_reverse_and_add(mpz_class start) {
    SeqPrinter<decltype([](auto& n){
        static size_t count = 0;
        return count++ != 100;
    }),
    decltype([](auto& n){
        auto reverse = reverse_num(n);
        auto sum = reverse + n;
        n = sum;
    }),
    decltype([](auto& n){
        return is_num_palindrom(n);
    })
    >()(start);
}


// Custom state example
struct FibState {
    mpz_class prev;
    mpz_class curr;
    size_t end = 100;
};
std::ostream& operator<<(std::ostream& os, const FibState& s) {
    os << s.curr;
    return os;
}

void print_fibonacci_seq() {
    SeqPrinter<
        decltype([](FibState& state){ 
            static size_t current = 0;
            return current++ != state.end;
        }), 
        decltype([](FibState& state){
            auto tmp = state.prev;
            state.prev = state.curr;
            state.curr = tmp + state.curr;
        }),
        decltype([](FibState& n){return true;}), 
        FibState>()(FibState{mpz_class(0), mpz_class(1)});
}

int main() {
    std::cout << "Fib: 100" << "\n";
    print_fibonacci_seq();
    std::cout << "\n";

    std::cout << "Reverse and add (find palindrome): 231 (100 deep)\n";
    print_reverse_and_add(mpz_class("231"));
    std::cout << "\n";

    std::cout << "Collatz seq: 44431\n";
    print_collatz_seq(mpz_class("44431"));
}