#include <iostream>

// NFA (gpt said it's not, but should check again)
// 162 page
// alphabet = "(())""

//    (         (       (       (
//   --->      --->    --->    --->      
// S       Q1       Q2      Q3      Q4
//   <---      <---    <---    <---      
//     )         )       )       )

// Pic. 13.5

template <char begin = '(', char end = ')'>
struct IsLang {
    constexpr bool operator()(const std::string_view& s) const {
        enum class State {
            S,Q1,Q2,Q3,Q4
        } cs = State::S;
        
        for(auto it = s.begin(), ite = s.end(); it!=ite; ++it){
            char sym = *it;
            if (sym != begin && sym != end) {
                return false;
            }
            switch(cs) {
                case State::S:
                    if (sym == begin){
                        cs = State::Q1;
                    } else {
                        return false;
                    }
                    break;
                case State::Q1:
                    cs = sym == end ? State::S : State::Q2;
                    break;
                case State::Q2:
                    cs = sym == end ? State::Q1 : State::Q3;
                    break;
                case State::Q3:
                    cs = sym == end ? State::Q2 : State::Q4;
                    break;
                case State::Q4:
                    if (sym == end) {
                        cs = State::Q3;
                    } else {
                        return false;
                    }
                    break;
            }
        }
        return cs == State::S;
    }
};

int main() {
    static_assert(IsLang{}("(((())))"));
    static_assert(IsLang{}("()()()()"));
    static_assert(IsLang{}("(())(())(((())))"));
    static_assert(!IsLang{}("((()))))"));
    static_assert(IsLang<'a','b'>{}("aaaabbbb"));
    static_assert(IsLang<'a','b'>{}("aabb"));
    static_assert(!IsLang<'a','b'>{}("abb"));
}