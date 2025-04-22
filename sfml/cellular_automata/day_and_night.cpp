#include <Automata.hpp>

struct DayAndNightCell: automata::GoLCell {
   void next(){
            if (state == State::Dead && (alive_neighbors == 3 || alive_neighbors == 6 || alive_neighbors == 7 || alive_neighbors == 8)) {
                set_state(State::Alive);
                return;
            }
            if (state == State::Alive && !(alive_neighbors == 3 || alive_neighbors == 4 || 
                alive_neighbors == 6 || alive_neighbors == 7 || alive_neighbors == 8)){
                set_state(State::Dead);
                return;
            }
    }
};


void DayAndNight() {
    using Cell = DayAndNightCell;

    constexpr size_t height = 80;
    constexpr size_t width = 150;
    auto custom_func = [](Cell::State s){return s == Cell::State::Alive ? Cell::State::Dead : Cell::State::Alive;};
    automata::SFMLVisualizer<
        automata::Field<height, width, Cell, automata::N8Init>,
        decltype(custom_func)> v;
    v.display();
}

int main() {
    DayAndNight();
    return 0;
}