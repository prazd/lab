#include <Automata.hpp>
#include <iostream>



enum class State: char {
    One,       
    Zero        
} state = State::Zero;


struct ElementaryCell: automata::VisualSFMLCell<2, ElementaryCell> {
    enum class State: char {
        One,       
        Zero        
    } state = State::Zero;

    static std::unordered_map<std::string, State> mgen(unsigned char rule) {
    auto binval = std::bitset<8>(rule).to_string();
    return {
        // 0b01011010
        {"111", binval[0] == '0' ? State::Zero : State::One}, // 0
        {"110", binval[1] == '0' ? State::Zero : State::One}, // 0
        {"101", binval[2] == '0' ? State::Zero : State::One}, // 0
        {"100", binval[3] == '0' ? State::Zero : State::One}, // 0
        {"011", binval[4] == '0' ? State::Zero : State::One}, // 0
        {"010", binval[5] == '0' ? State::Zero : State::One}, // 0
        {"001", binval[6] == '0' ? State::Zero : State::One}, // 0
        {"000", binval[7] == '0' ? State::Zero : State::One}, // 0
    }; 
    }
    static inline std::unordered_map<std::string, State> rule = mgen(88);

    State next_state = State::Zero;
    
    void off() {
        set_state(State::Zero);
    }
    static State get_default_state() {
        return State::Zero;
    }

    void set_state(State s) {
        state = s;
        if (state == State::Zero) {
            rs.setFillColor(sf::Color::Black);
            rs.setOutlineColor(sf::Color::White);
        } else {
            rs.setFillColor(sf::Color::White);
            rs.setOutlineColor(sf::Color::Black);
        }
    }

    State get_state() const {
        return state;
    }

    void prepare() {
        std::string key;
        key += std::to_string(static_cast<char>(neighbours[0]->state));
        key += std::to_string(static_cast<char>(state)); 
        key += std::to_string(static_cast<char>(neighbours[1]->state));
        next_state = rule[key];
    }

    void next() {
        if (state == State::Zero){
            set_state(next_state);
        }
        
    }

};

struct N2Init{
        template <size_t width, size_t height>
        void operator()(auto (&cells) [height][width]){
          for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                cells[i][j].neighbours[0] = &cells[(i - 1 + height) % height][(j - 1 + width) % width];
                cells[i][j].neighbours[1] = &cells[(i - 1 + height) % height][(j + 1) % width];
            }
          }
        }
};

void Elementary() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    auto custom_func = [](ElementaryCell::State s){return s == ElementaryCell::State::One ? ElementaryCell::State::Zero : ElementaryCell::State::One;};
    automata::SFMLVisualizer<
        automata::Field<height, width, ElementaryCell, N2Init>,
        decltype(custom_func)> v;
    v.display();
}

int main() {
    Elementary();
    return 0;
}