#include <Automata.hpp>

class HLCell {
    size_t alive_neighbors;
public:
    std::array<HLCell*, 8> neighbours;
    enum class State: char {
        Alive,
        Dead
    } state = State::Dead;
    sf::RectangleShape rs;
    void set_state(State s);
    State get_state() const;
    void off();
    void prepare();
    void next();
    static State get_default_state();
};

void HLCell::off() {
    set_state(State::Dead);
}

HLCell::State HLCell::get_default_state() {
    return State::Alive;
}

void HLCell::set_state(State s) {
    state = s;
    if (state == State::Alive) {
        rs.setFillColor(sf::Color::White);
    } else {
        rs.setFillColor(sf::Color::Black);
    }
}
HLCell::State HLCell::get_state() const {
    return state;
}

void HLCell::prepare() {
    size_t res{};
    for (auto el: neighbours) {
        if (el != nullptr && el->get_state() == State::Alive){
            ++res;
        }
    }
    alive_neighbors = res;
}

void HLCell::next() {
    // Правила HighLife:
    // Клетка остаётся живой, если у неё 2 или 3 живых соседа (то же, что в Game of Life).
    // Пустая клетка становится живой (рождается), если у неё 6 живых соседей (в Game of Life рождение происходит при 3 живых соседях).

    if (state == State::Dead && (alive_neighbors == 6 || alive_neighbors == 3)) {
        set_state(State::Alive);
        return;
    }
    if (state == State::Alive && (alive_neighbors < 2 || alive_neighbors > 3)){
        set_state(State::Dead);
        return;
    }

}

void HighLife() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    auto custom_func = [](HLCell::State s){return s == HLCell::State::Alive ? HLCell::State::Dead : HLCell::State::Alive;};
    automata::SFMLVisualizer<
        automata::Field<height, width, HLCell>, 
        decltype(custom_func)> v;    
    v.display(custom_func);
}

int main() {
    HighLife();
    return 0;
}