#include <Automata.hpp>

struct WireworldCell: automata::VisualSFMLCell<8, WireworldCell> {
    size_t head_neighbors;
    
    enum class State: char {
        Empty,       
        Conductor,  
        Head,       
        Tail         
    } state = State::Empty;
    
    void off() {
        set_state(State::Empty);
    }
    static State get_default_state() {
        return State::Conductor;
    }
    
    void set_state(State s) {
        state = s;
        if (state == State::Head) {
            rs.setFillColor(sf::Color::Red);
        } else if (state == State::Tail) {
            rs.setFillColor(sf::Color::Blue);
        } else if (state == State::Conductor) {
            rs.setFillColor(sf::Color::White);
        } else {
            rs.setFillColor(sf::Color::Black);
        }
    }

    State get_state() const {
        return state;
    }

    void prepare() {
        size_t head_count = 0;
        for (auto el: neighbours) {
            if (el != nullptr && el->get_state() == State::Head) {
                ++head_count;
            }
        }
        head_neighbors = head_count;
    }

    void next() {
    switch (state) {
            case State::Head:
                set_state(State::Tail);  // Голова переходит в хвост
                break;
            case State::Tail:
                set_state(State::Conductor);  // Хвост переходит в проводник
                break;
            case State::Conductor:
                if (head_neighbors == 1 || head_neighbors == 2) {
                    set_state(State::Head);  // Если 1 или 2 соседа - становится головой
                }
                break;
            default:
                break;
        }
    }
};

void Wireworld() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    auto switch_func = [](WireworldCell::State s){
        switch(s) {
            case WireworldCell::State::Empty:
                return WireworldCell::State::Conductor;
            case WireworldCell::State::Conductor:
                return WireworldCell::State::Head;
            case WireworldCell::State::Head:
                return WireworldCell::State::Tail;
            default:
                return WireworldCell::State::Empty;
        }
    };
    automata::SFMLVisualizer<
        automata::Field<height, width, WireworldCell, automata::N8Init>,
        decltype(switch_func)> v;
    v.display();
}

int main() {
    Wireworld();
    return 0;
}