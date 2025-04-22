#include <Automata.hpp>

struct CoralGrowthCell: automata::GoLCell {
    void next(){
            if (state == State::Dead && alive_neighbors == 3) {
                set_state(State::Alive);
            } else if (state == State::Alive && !(alive_neighbors > 3 && alive_neighbors < 9)){
                set_state(State::Dead);
            }
        }
   
};

void CoralGrowth() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    using Cell = CoralGrowthCell;
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
    CoralGrowth();
    return 0;
}