#include <Automata.hpp>

class GoLCell {
    size_t alive_neighbors;
public:
    std::array<GoLCell*, 8> neighbours;
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

void GoLCell::off() {
    set_state(State::Dead);
}

GoLCell::State GoLCell::get_default_state() {
    return State::Alive;
}

void GoLCell::set_state(State s) {
    state = s;
    if (state == State::Alive) {
        rs.setFillColor(sf::Color::White);
    } else {
        rs.setFillColor(sf::Color::Black);
    }
}

GoLCell::State GoLCell::get_state() const {
    return state;
}

void GoLCell::prepare() {
    size_t res{};
    for (auto el: neighbours) {
        if (el != nullptr && el->get_state() == State::Alive){
            ++res;
        }
    }
    alive_neighbors = res;
}

void GoLCell::next() {
    // в пустой (мёртвой) клетке, с которой соседствуют три живые клетки, зарождается жизнь;
    // если у живой клетки есть две или три живые соседки, то эта клетка продолжает жить; в противном случае (если живых соседей меньше двух или больше трёх) клетка умирает («от одиночества» или «от перенаселённости»)
    if (state == State::Dead && alive_neighbors == 3) {
        set_state(State::Alive);
        return;
    }
    if (state == State::Alive && (alive_neighbors < 2 || alive_neighbors > 3)){
        set_state(State::Dead);
        return;
    }
}

void GameOfLife() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    auto custom_func = [](GoLCell::State s){return s == GoLCell::State::Alive ? GoLCell::State::Dead : GoLCell::State::Alive;};
    automata::SFMLVisualizer<
        automata::Field<height, width, GoLCell, automata::N8Init>,
        decltype(custom_func)> v;
        
    v.display();
}

int main() {
    GameOfLife();
    return 0;
}