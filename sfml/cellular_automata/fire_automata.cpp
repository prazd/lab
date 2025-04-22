#include <Automata.hpp>

struct FireAutomataCell: automata::VisualSFMLCell<8, FireAutomataCell> {
    size_t fired_neighbours{};
    
    enum class State {
        Empty,
        Tree,
        Fire,
        Ash
    } state = State::Empty;
    
    void off() {
        set_state(State::Empty);
    }
    static State get_default_state() {
        return State::Empty;
    }
    
    void set_state(State s) {
        state = s;
        if (state == State::Fire) {
            rs.setFillColor(sf::Color::Red);
        } else if (state == State::Tree) {
            rs.setFillColor(sf::Color::Green);
        } else if (state == State::Ash) {
            rs.setFillColor(sf::Color::White);
        } else {
            rs.setFillColor(sf::Color::Black);
        }
    }

    State get_state() const {
        return state;
    }

    void prepare() {
        size_t alive = 0;
        for (auto el: neighbours) {
            if (el->get_state() == State::Fire) {
                ++alive;
            }
        }
        fired_neighbours = alive;
    }

    void next() {
    if (state == State::Tree && fired_neighbours > 0) {
        if (automata::Random<0,100>::roll() < 70) { // 70% вероятность возгорания
            set_state(State::Fire);
        }
    } else if (state == State::Fire) {
        set_state(State::Ash);
    } else if (state == State::Empty) {
        if (automata::Random<0,100>::roll() < 5) { // 5% вероятность роста дерева
            set_state(State::Tree);
        }
    }
    }
};

void FireAutomata() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    using Cell = FireAutomataCell;
    auto switch_func = [](Cell::State s){
        switch(s) {
            case Cell::State::Empty:    return Cell::State::Tree;
            case Cell::State::Tree:     return Cell::State::Fire;
            case Cell::State::Fire:     return Cell::State::Ash;
            case Cell::State::Ash:      return Cell::State::Empty;
        }
    };
    automata::SFMLVisualizer<
        automata::Field<height, width, Cell, automata::N8Init>,
        decltype(switch_func)> v;
    v.display();
}

int main() {
    FireAutomata();
    return 0;
}