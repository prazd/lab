#include <Automata.hpp>

struct QuadLifeCell: automata::VisualSFMLCell<8, QuadLifeCell> {
    char alive_a_n{};
    char alive_b_n{};
    char alive_c_n{};
    
    enum class State: char {
        A,
        B,
        C,
        Dead
    } state = State::Dead;
    
    void off() {
        set_state(State::Dead);
    }
    static State get_default_state() {
        return State::Dead;
    }
    
    void set_state(State s) {
        state = s;
        if (state == State::A) {
            rs.setFillColor(sf::Color::Red);
        } else if (state == State::B) {
            rs.setFillColor(sf::Color::Green);
        } else if (state == State::C) {
            rs.setFillColor(sf::Color::Blue);
        } else {
            rs.setFillColor(sf::Color::Black);
        }
    }

    State get_state() const {
        return state;
    }

    void prepare() {
        reset();
        for (auto el: neighbours) {
            switch(el->get_state()){
                case State::A: ++alive_a_n; break;
                case State::B: ++alive_b_n; break;
                case State::C: ++alive_c_n; break;
                default: break;
            }
        }
    }

    void reset() {
        alive_a_n = 0;
        alive_b_n = 0;
        alive_c_n = 0;
    }

    void next() {
        auto sum = alive_a_n + alive_b_n + alive_c_n;
        if (state == State::Dead && sum == 3) {
            if (alive_a_n > alive_b_n && alive_a_n > alive_c_n) {
                set_state(State::A);
            } else if (alive_b_n > alive_a_n && alive_b_n > alive_c_n) {
                set_state(State::B);
            } else {
                set_state(State::C);
            }  
        } else if (state != State::Dead && (sum < 2 || sum > 3)) set_state(State::Dead);
    }
};

void QuadLife() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    using Cell = QuadLifeCell;
    auto switch_func = [](Cell::State s){
        switch(s) {
            case Cell::State::A:    return Cell::State::B;
            case Cell::State::B:    return Cell::State::C;
            case Cell::State::C:    return Cell::State::Dead;
            case Cell::State::Dead: return Cell::State::A;
        }
    };
    automata::SFMLVisualizer<
        automata::Field<height, width, Cell, automata::N8Init>,
        decltype(switch_func)> v;
    v.display();
}

int main() {
    QuadLife();
    return 0;
}