#include <iostream>

template <char f = '0', char s = '1'>
struct NFA {
    constexpr bool operator()(const std::string& value) {
        enum class State: char {
            Q0, Q1, Q2, Q3
        } state = State::Q0;

        for(auto& el: value) {
            if (el == f) {
                if (state == State::Q0 || state == State::Q1) {
                    state = State::Q1;
                } else if (state == State::Q2) {
                    state = State::Q3;
                }
            } else if (el == s) {
                if (state == State::Q0) {
                    state = State::Q2;
                } else if (state == State::Q1) {
                    state = State::Q3;
                } else if (state == State::Q2) {
                    state = State::Q2;
                }
            }
            if (state == State::Q3) {
                return true;
            }
        }
        return state == State::Q3;
    }
    
};

int main() {
    static_assert(NFA{}("01"));
    static_assert(NFA{}("10"));
    static_assert(NFA{}("1010"));
    static_assert(NFA{}("101001"));
    static_assert(NFA{}("1000000"));
    static_assert(NFA{}("1000001"));
    static_assert(!NFA{}("11"));
    static_assert(!NFA{}("00"));
}