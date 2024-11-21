#include <Automata.hpp>

struct HLCell: automata::GoLCell {
    void next(){
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
};

void HighLife() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    auto switch_state = [](HLCell::State s){return s == HLCell::State::Alive ? HLCell::State::Dead : HLCell::State::Alive;};
    automata::SFMLVisualizer<
        automata::Field<height, width, HLCell, automata::N8Init>,
        decltype(switch_state)> v;
    
    v.display();
}

int main() {
    HighLife();
    return 0;
}