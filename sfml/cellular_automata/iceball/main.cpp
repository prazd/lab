#include <Automata.hpp>
#include <random>

template <int begin, int end>
class Random {
    static inline std::random_device rd;
    static inline std::mt19937 gen{rd()};
    static inline std::uniform_int_distribution<int> dis = std::uniform_int_distribution<int>(begin, end);
public:
    static int roll() {
        return dis(gen);
    }
};

struct IceBallCell: automata::VisualSFMLCell<8, IceBallCell> {
    size_t iced_neighbours{};
    
    enum class State {
        Empty,
        Ice
    } state = State::Empty;
    
    void off() {
        set_state(State::Empty);
    }
    static State get_default_state() {
        return State::Empty;
    }
    
    void set_state(State s) {
        state = s;
        if (state == State::Ice) {
            rs.setFillColor(sf::Color::White);
        } else {
            rs.setFillColor(sf::Color::Blue);
        }
    }

    State get_state() const {
        return state;
    }

    void prepare() {
        size_t iced = 0;
        for (auto el: neighbours) {
            if (el->get_state() == State::Ice) {
                ++iced;
            }
        }
        iced_neighbours = iced;
    }

    void next() {
        if (state == State::Empty && iced_neighbours > 0 && iced_neighbours <= 3) {
            if (Random<0,100>::roll() < iced_neighbours * 13) { 
                set_state(State::Ice);
            }
        }
        // if (state == State::Ice && Random<0, 100>::roll() < 5) {
        //     set_state(State::Empty);
        // }
    }

};

void IceBall() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    using Cell = IceBallCell;
    auto switch_func = [](Cell::State s){
        switch(s) {
            case Cell::State::Empty:    return Cell::State::Ice;
            case Cell::State::Ice:     return Cell::State::Empty;
        }
    };
    automata::SFMLVisualizer<
        automata::Field<height, width, Cell, automata::N8Init>,
        decltype(switch_func)> v(sf::Color::Blue, sf::Color::Black);
    v.display();
}

int main() {
    IceBall();
    return 0;
}