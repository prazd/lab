#include <Automata.hpp>

struct MazeCell: automata::GoLCell {
   void next(){
            if (state == State::Dead && (alive_neighbors == 3)) {
                set_state(State::Alive);
                return;
            }
            if (state == State::Alive && (alive_neighbors > 1 && alive_neighbors < 6)){
                set_state(State::Dead);
                return;
            }
    }
};


void Maze() {
    using Cell = MazeCell;

    constexpr size_t height = 80;
    constexpr size_t width = 150;
    auto custom_func = [](Cell::State s){return s == Cell::State::Alive ? Cell::State::Dead : Cell::State::Alive;};
    automata::SFMLVisualizer<
        automata::Field<height, width, Cell, automata::N8Init>,
        decltype(custom_func)> v;
    v.display();
}

int main() {
    Maze();
    return 0;
}