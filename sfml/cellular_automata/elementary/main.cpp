#include <Automata.hpp>
#include <iostream>

template <unsigned char rule = 90>
struct ElementaryCell: automata::VisualSFMLCell<2, ElementaryCell<rule>> {
    
    enum class State: char {
        One,       
        Zero        
    } state = State::Zero;

    static std::unordered_map<std::string, State> mgen() {
        auto binval = std::bitset<CHAR_BIT>(rule).to_string();
        auto set_state = [](char s){return s == '0' ? State::Zero : State:: One;};
        return {
            {"111", set_state(binval[0])},
            {"110", set_state(binval[1])},
            {"101", set_state(binval[2])},
            {"100", set_state(binval[3])},
            {"011", set_state(binval[4])},
            {"010", set_state(binval[5])},
            {"001", set_state(binval[6])},
            {"000", set_state(binval[7])},
        }; 
    }

    static inline std::unordered_map<std::string, State> rmap = mgen();

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
            this->rs.setFillColor(sf::Color::Black);
            this->rs.setOutlineColor(sf::Color::White);
        } else {
            this->rs.setFillColor(sf::Color::White);
            this->rs.setOutlineColor(sf::Color::Black);
        }
    }

    State get_state() const {
        return state;
    }

    void prepare() {
        std::string key;
        key += std::to_string(static_cast<char>(this->neighbours[0]->state));
        key += std::to_string(static_cast<char>(state)); 
        key += std::to_string(static_cast<char>(this->neighbours[1]->state));
        this->next_state = rmap[key];
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
    constexpr unsigned char rule = 90;

    using ECell = ElementaryCell<rule>;

    auto custom_func = [](ECell::State s){return s == ECell::State::One ? ECell::State::Zero : ECell::State::One;};
    automata::SFMLVisualizer<
        automata::Field<height, width, ECell, N2Init>,
        decltype(custom_func)> v;
    v.display();
}

int main() {
    Elementary();
    return 0;
}