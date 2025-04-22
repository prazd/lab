#include <Automata.hpp>

struct BriansBrainCell: automata::VisualSFMLCell<8, BriansBrainCell> {
    size_t alive_neighbours{};
    
    enum class State: char {
        On,
        Dying,
        Off
    } state = State::Off;
    
    void off() {
        set_state(State::Off);
    }
    static State get_default_state() {
        return State::Off;
    }
    
    void set_state(State s) {
        state = s;
        if (state == State::On) {
            rs.setFillColor(sf::Color::Green);
        } else if (state == State::Dying) {
            rs.setFillColor(sf::Color::Yellow);
        }else {
            rs.setFillColor(sf::Color::Black);
        }
    }

    State get_state() const {
        return state;
    }

    void prepare() {
        size_t alive = 0;
        for (auto el: neighbours) {
            if (el->get_state() == State::On) {
                ++alive;
            }
        }
        alive_neighbours = alive;
    }

    void next() {
    switch (state) {
            case State::On:
                set_state(State::Dying); 
                break;
            case State::Dying:
                set_state(State::Off);
                break;
            case State::Off:
                if (alive_neighbours == 2) {
                    set_state(State::On);
                }
                break;
            default:
                break;
        }
    }
};

void BrianCell() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    using BCell = BriansBrainCell;
    auto switch_func = [](BCell::State s){
        switch(s) {
            case BCell::State::On:    return BCell::State::Dying;
            case BCell::State::Dying: return BCell::State::Off;
            case BCell::State::Off:   return BCell::State::On;
        }
    };
    automata::SFMLVisualizer<
        automata::Field<height, width, BCell, automata::N8Init>,
        decltype(switch_func)> v;
    v.display();
}

int main() {
    BrianCell();
    return 0;
}