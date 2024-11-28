#include <Automata.hpp>
#include <iostream>

struct ReplicatorCell: automata::VisualSFMLCell<8, ReplicatorCell> {
    size_t alive_neighbours{};
    
    enum class State: char {
        Alive,
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
        if (state == State::Dead) {
            rs.setFillColor(sf::Color::Black);
        } else {
            rs.setFillColor(sf::Color::White);
        }
    }

    State get_state() const {
        return state;
    }

    void prepare() {
        size_t alive = 0;
        for (auto el: neighbours) {
            if (el->get_state() == State::Alive) {
                ++alive;
            }
        }
        alive_neighbours = alive;
    }

    void next() {
        if (state == State::Alive && alive_neighbours > 0 && (alive_neighbours & 1)) {
            set_state(State::Dead);
        } else if (state == State::Dead && alive_neighbours > 0 && !(alive_neighbours & 1)) {
            set_state(State::Alive);
        }
    }
};

void Replicator() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    using Cell = ReplicatorCell;
    auto switch_func = [](Cell::State s){
        switch(s) {
            case Cell::State::Alive:    return Cell::State::Dead;
            case Cell::State::Dead:     return Cell::State::Alive;
        }
    };
    automata::SFMLVisualizer<
        automata::Field<height, width, Cell, automata::N8Init>,
        decltype(switch_func)> v;
    v.display();
}

int main() {
    Replicator();
    return 0;
}