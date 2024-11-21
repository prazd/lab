#include <Automata.hpp>

void GameOfLife() {
    using GoLCell = automata::GoLCell;

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