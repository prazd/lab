#include <Automata.hpp>

struct AntCell: automata::VisualSFMLCell<4, AntCell>{
     enum class State: char {
        Black,       
        White,
        Ant
    } state = State::White;

    // unsigned char ant_dir = 0;
    unsigned char ant_dir = 0;

    State prev_state = State::White;
    State next_state;

    bool updated = false;

    void set_state(State s){
        prev_state = state;
        update_state(s);
    };

    void update_state(State s) {
        state = s;
        if (state == State::Black) {
            rs.setFillColor(sf::Color::Black);
            rs.setOutlineColor(sf::Color::White);
        } else if (state == State::White) {
            rs.setFillColor(sf::Color::White);
            rs.setOutlineColor(sf::Color::Black);
        } else {
            rs.setFillColor(sf::Color::Red);
        }
    }

    static State get_default_state() {
        return State::White;
    }
    
    State get_state(){ return state;};

    void off(){ set_state(State::White);};

    void prepare(){
        if (updated) {
            return;
        }

        if (state != State::Ant){
            next_state = state;
            prev_state = state;
            updated = true;
            return;
        };

        next_state = (prev_state == State::Black ? State::White : State::Black);
        updated = true;

        if (prev_state == State::White) {
            ant_dir = (ant_dir + 1) % 4;
        } else if (prev_state == State::Black) {
            ant_dir = (ant_dir + 4 - 1) % 4;
        }

        auto tmp = neighbours[ant_dir]->state;
        neighbours[ant_dir]->prev_state = tmp;
        neighbours[ant_dir]->next_state = State::Ant;
        neighbours[ant_dir]->ant_dir = ant_dir;
        neighbours[ant_dir]->updated = true;
    }

    void next(){
        update_state(next_state);
        updated = false;
    }

};


struct N4Init{
        template <size_t width, size_t height>
        void operator()(auto (&cells) [height][width]){
          for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                cells[i][j].neighbours[0] = &cells[i][(j - 1 + width) % width];
                cells[i][j].neighbours[1] = &cells[(i - 1 + height) % height][j];
                cells[i][j].neighbours[2] = &cells[i][(j + 1) % width];
                cells[i][j].neighbours[3] = &cells[(i + 1) % height][j];
            }
        }
    }
};


void Ant() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    using ACell = AntCell;

    auto custom_func = [](ACell::State s){
        if (s == ACell::State::White) {
            return ACell::State::Black;
        } else if (s == ACell::State::Black) {
            return ACell::State::Ant;
        } else {
            return ACell::State::White;
        }
    };

    automata::SFMLVisualizer<
        automata::Field<height, width, ACell, N4Init>,
        decltype(custom_func)> v(sf::Color::White, sf::Color::Black);
    v.display();
}

int main() {
    Ant();
    return 0;
}