#include <iostream>

int only_ints(int x) {
    return x;
}

template <typename T>
T only_ints(T x) = delete;

int main() {
    const int& x = 2;
    only_ints(2);
    only_ints(x);
    
    // char c = '1';
    // only_ints(c);   // Compilation Error.
    // only_ints(2.5); // Explicitly deleted
}