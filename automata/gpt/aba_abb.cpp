#include <iostream>
#include <optional>

// alphabet = "aba/abb""
struct IsLang {
    constexpr bool operator()(const std::string& s) const {
        enum class State {
            S,
            Q1,Q2,Q3, // aba
            Q4,Q5,Q6  // abb
        };

        std::tuple<std::optional<State>, std::optional<State>> state = std::make_tuple(State::S, std::optional<State>{});

        for(auto it = s.begin(), ite = s.end(); it!=ite; ++it){
            char sym = *it;
            if (sym != 'a' && sym != 'b') {
                return false;
            }
            auto [fstate, sstate] = state;
            if (fstate && !(sstate)) {
                if (fstate == State::S || fstate == State::Q3 || fstate == State::Q6) {
                    if (sym == 'a') {
                        state = std::make_tuple(State::Q1, State::Q4);
                    }
                } else {
                    return false;
                }
            }

            if (fstate && sstate) {
                if (fstate == State::Q1 && sstate == State::Q4) {
                    if (sym == 'b') {
                        state = std::make_tuple(State::Q2, State::Q5);
                    } else {
                       return false;
                    }
                } else if (fstate == State::Q2 && sstate == State::Q5) {
                    if (sym == 'a') {
                        state = std::make_tuple(State::Q3, std::optional<State>{});
                    } else if (sym == 'b') {
                        state = std::make_tuple(State::Q6, std::optional<State>{});
                    } else {
                       return false;
                    }
                }
            }
        }
        auto [result, _] = state;
        if (result) {
            return (result == State::Q3 || result == State::Q6);
        }
        return false;
    }
};

int main() {
    static_assert(IsLang{}("aba"));
    static_assert(IsLang{}("abb"));
    static_assert(!IsLang{}("aab"));

    static_assert(IsLang{}("abbaba"));
    static_assert(IsLang{}("abbabbabaabb"));
    static_assert(!IsLang{}("abbabbabaabbab"));
    static_assert(!IsLang{}(""));
}