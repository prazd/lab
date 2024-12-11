#include <Automata.hpp>

struct VoteAutomataCell: automata::VisualSFMLCell<8, VoteAutomataCell> {
    size_t agree_neighbours{};
    static size_t inline mid_neighbours = 4;
    
    enum class State {
        Agree,
        Disagree
    } state = State::Disagree;
    
    void off() {
        set_state(State::Disagree);
    }
    static State get_default_state() {
        return State::Disagree;
    }
    
    void set_state(State s) {
        state = s;
        if (state == State::Disagree) {
            rs.setFillColor(sf::Color::Black);
        } else {
            rs.setFillColor(sf::Color::White);
        }
    }

    State get_state() const {
        return state;
    }

    void prepare() {
        size_t agree = 0;
        for (auto el: neighbours) {
            if (el->get_state() == State::Agree) {
                ++agree;
            }
        }
        agree_neighbours = agree;
    }

    void next() {
        if (agree_neighbours > mid_neighbours){
            set_state(State::Agree);
        } else if (agree_neighbours < mid_neighbours) {
            set_state(State::Disagree);
        } else if (agree_neighbours == mid_neighbours) {
            auto res = automata::Random<0,100>::roll();
            if (res >= 50) {
                set_state(State::Disagree);
            } else {
                set_state(State::Agree);
            }
        }
    }
};

void VoteAutomata() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    using Cell = VoteAutomataCell;
    auto switch_func = [](Cell::State s){
        switch(s) {
            case Cell::State::Disagree:    return Cell::State::Agree;
            case Cell::State::Agree:       return Cell::State::Disagree;
        }
    };
    automata::SFMLVisualizer<
        automata::Field<height, width, Cell, automata::N8Init>,
        decltype(switch_func)> v;
    v.display();
}

int main() {
    VoteAutomata();
    return 0;
}