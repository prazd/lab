#include <iostream>
#include <cassert>
#include <stack>

// DFA
// 159 page

// alphabet = {x, y} 
template <char begin, char end>
struct IsLang {
    constexpr bool operator()(const std::string& s) const {
        enum class State {
            S,
            Q1,
            Q2
        } current_state = State::S;
        for(auto it = s.begin(), ite = s.end();it!=ite;++it){
            char sym = *it;
            if (sym != begin && sym != end) {
                return false;
            }
            switch (current_state) {
                case State::S:
                    if (sym == end) {
                        current_state = State::Q1;
                    }
                    break;
                case State::Q1:
                    current_state = State::Q2;
                    break;
                case State::Q2:
                    break;
            }
        }
        return current_state == State::Q1;
    }
};

int main() {
    static_assert(IsLang<'a','b'>{}("aaab"));
    static_assert(IsLang<'x','y'>{}("xxxy"));
    static_assert(!IsLang<'x','y'>{}("xxxyx"));
    static_assert(!IsLang<'x','y'>{}("xyxyx"));
    static_assert(IsLang<'z','h'>{}("zh"));
    static_assert(IsLang<'z','h'>{}("h"));
}