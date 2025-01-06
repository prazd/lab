#include <Automata.hpp>

struct Levialdi: automata::GoLCell {
    decltype(state) next_state;
    void prepare() {
        bool n0 = (neighbours[0]->get_state() == State::Alive);
        bool n3 = (neighbours[3]->get_state() == State::Alive);
        bool n6 = (neighbours[6]->get_state() == State::Alive);
        next_state = ((state == State::Alive && (n0 || n6 || n3)) || (n0 && n3)) ? State::Alive : State::Dead;
    }
    void next(){
        set_state(next_state);
    }
};

void LevialdiAutomata() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    auto switch_state = [](Levialdi::State s){return s == Levialdi::State::Alive ? Levialdi::State::Dead : Levialdi::State::Alive;};
    automata::SFMLVisualizer<
        automata::Field<height, width, Levialdi, automata::N8Init>,
        decltype(switch_state)> v;
    
    v.display();
}

int main() {
    LevialdiAutomata();
    return 0;
}