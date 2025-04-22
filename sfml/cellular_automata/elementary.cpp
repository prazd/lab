#include <Automata.hpp>
#include <iostream>

template <unsigned char rule>
struct ElementaryCellBase{
    enum class State: char {
        One,       
        Zero        
    };

    static std::unordered_map<std::string, State> mgen() {
        auto binval = std::bitset<CHAR_BIT>(rule).to_string();
        auto set_state = [](char s){return s == '0' ? State::Zero : State::One;};
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
    static State get_default_state() {
        return State::Zero;
    }

};

template <unsigned char rule>
struct ECell: ElementaryCellBase<rule>, automata::VisualSFMLCell<3, ECell<rule>> {
    using State = ElementaryCellBase<rule>::State;

    State state = State::Zero;
    State next_state = State::Zero;

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

    void off() {
        set_state(State::Zero);
    }

    void prepare() {
        std::string key;
        key += this->neighbours[0]->state == State::Zero ? "0" : "1";
        key += this->neighbours[1]->state == State::Zero ? "0" : "1";
        key += this->neighbours[2]->state == State::Zero ? "0" : "1";
        next_state = this->rmap[key];
    }

    void next() {
        if (state == State::Zero) {
            set_state(next_state);
        }
    }
};


// ...
template <unsigned char rule, size_t fheight, size_t fwidth>
struct SyncECell: ElementaryCellBase<rule>, automata::VisualSFMLCell<3, SyncECell<rule, fheight, fwidth>> {
    using State = ElementaryCellBase<rule>::State;

    State state = State::Zero;
    State next_state = State::Zero;

    static inline size_t cheight{};
    static inline size_t cwidth{};
    size_t height{};
    size_t width{};

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

    void off() {
        set_state(State::Zero);
    }

    void prepare() {
        std::string key;
        key += this->neighbours[0]->state == State::Zero ? "0" : "1";
        key += this->neighbours[1]->state == State::Zero ? "0" : "1";
        key += this->neighbours[2]->state == State::Zero ? "0" : "1";
        next_state = this->rmap[key];
    }

    void next() {
        if (height == (cheight+1)%fheight){
            set_state(next_state);
        }
        if (width == fwidth-1 && height == fheight-1){
            cheight = (cheight + 1) % fheight;
        }
    }
};


struct N3InitSync{
        template <size_t width, size_t height>
        void operator()(auto (&cells) [height][width]){
          for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                cells[i][j].height = i;
                cells[i][j].width = j;
                cells[i][j].neighbours[0] = &cells[(i - 1 + height) % height][(j - 1 + width) % width];
                cells[i][j].neighbours[1] = &cells[(i - 1 + height) % height][j];
                cells[i][j].neighbours[2] = &cells[(i - 1 + height) % height][(j + 1) % width];
            }
          }
    }
};

struct N3Init{
        template <size_t width, size_t height>
        void operator()(auto (&cells) [height][width]){
          for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                cells[i][j].neighbours[0] = &cells[(i - 1 + height) % height][(j - 1 + width) % width];
                cells[i][j].neighbours[1] = &cells[(i - 1 + height) % height][j];
                cells[i][j].neighbours[2] = &cells[(i - 1 + height) % height][(j + 1) % width];
            }
          }
    }
};


void run_sync() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    constexpr unsigned char rule = 99;

    using ECell = SyncECell<rule, height, width>;

    auto custom_func = [](ECell::State s){return s == ECell::State::One ? ECell::State::Zero : ECell::State::One;};
    automata::SFMLVisualizer<
        automata::Field<height, width, ECell, N3InitSync>,
        decltype(custom_func)> v;
    v.display();
}

void run_default() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    constexpr unsigned char rule = 30;

    using ECell = ECell<rule>;

    auto custom_func = [](ECell::State s){return s == ECell::State::One ? ECell::State::Zero : ECell::State::One;};
    automata::SFMLVisualizer<
        automata::Field<height, width, ECell, N3Init>,
        decltype(custom_func)> v;
    v.display();
}

int main() {
    run_default();
    return 0;
}