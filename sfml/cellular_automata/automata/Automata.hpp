#ifndef AUTOMATA_HPP
#define AUTOMATA_HPP
#include <SFML/Graphics.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>

// TODO: refactor (generalize)
namespace automata {
    template <typename T>
    concept CellLifecycle = requires(T cell) {
        { cell.prepare() } -> std::same_as<void>;
        { cell.next() } -> std::same_as<void>;
        { cell.get_state() } -> std::same_as<typename T::State>;
        { cell.set_state(typename T::State{}) } -> std::same_as<void>;
        { cell.off() } -> std::same_as<void>;
        { T::get_default_state() } -> std::same_as<typename T::State>;
    };

    template <size_t n, typename Cell>
    struct BaseCell {
        std::array<Cell*, n> neighbours;
    };

    template <size_t n, typename Cell>
    struct VisualSFMLCell: BaseCell<n, Cell> {
        sf::RectangleShape rs;
    };

    struct GoLCell: automata::VisualSFMLCell<8, GoLCell> {
    size_t alive_neighbors;
    enum class State: char {
        Alive,
        Dead
    } state = State::Dead;
    public:
        void set_state(State s){state = s; rs.setFillColor(state == State::Alive ? (sf::Color::White):(sf::Color::Black));};
        State get_state(){ return state; };
        void off(){ set_state(State::Dead);};
        static State get_default_state(){return State::Alive;};
        void prepare(){
            size_t res{};
                for (auto el: neighbours) {
                    if (el != nullptr && el->get_state() == State::Alive){
                    ++res;
                }
            }
            alive_neighbors = res;
        }
        void next(){
            if (state == State::Dead && alive_neighbors == 3) {
                set_state(State::Alive);
                return;
            }
            if (state == State::Alive && (alive_neighbors < 2 || alive_neighbors > 3)){
                set_state(State::Dead);
                return;
            }
        }
    };

    struct N8Init{
        template <size_t width, size_t height>
        void operator()(auto (&cells) [height][width]){
          for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                // [ [ 0 § 0 0 0 0 0 ] ]
                // [ [ 0 0 0 0 0 0 0 ] ]
                // [ [ 0 0 0 0 0 0 0 ] ]
                cells[i][j].neighbours[0] = &cells[i][(j - 1 + width) % width];
                cells[i][j].neighbours[1] = &cells[i][(j + 1) % width];
                cells[i][j].neighbours[2] = &cells[(i - 1 + height) % height][j];
                cells[i][j].neighbours[3] = &cells[(i + 1) % height][j];

                cells[i][j].neighbours[4] = &cells[(i - 1 + height) % height][(j - 1 + width) % width];
                cells[i][j].neighbours[5] = &cells[(i - 1 + height) % height][(j + 1) % width];
                cells[i][j].neighbours[6] = &cells[(i + 1) % height][(j - 1 + width) % width];
                cells[i][j].neighbours[7] = &cells[(i + 1) % height][(j + 1) % width];
            }
          }
        }
    };



    template <size_t height, size_t width, CellLifecycle Cell, typename InitFuncObj>
    class Field {
    public:
        [[no_unique_address]] InitFuncObj init;
        static inline Cell cells[height][width];

        Field() {
            init(cells);
        }

        void next_gen() {
            for(auto& row: cells)
                for(auto& cell: row)
                    cell.prepare();

             for(auto& row: cells)
                for(auto& cell: row)
                    cell.next();
        }

        void flush() {
            for(auto& row: cells)
                for(auto& cell: row)
                    cell.off();
        }

        decltype(auto) get_default_state() {
            return Cell::get_default_state();
        }

        constexpr size_t get_height() const {
            return height;
        }

        constexpr size_t get_width() const {
            return width;
        }
    };

    template <typename Field,  typename SwitchStateFunc, unsigned int cell_sz = 10>
    struct SFMLVisualizer{
        static inline auto BACKGROUND_COLOR = sf::Color::White;
        static inline const float cell_size = static_cast<float>(cell_sz);

        using Cell = typename std::remove_extent<typename std::remove_extent<decltype(Field::cells)>::type>::type;

        static inline std::array<Cell*, 20> cache;
        Cell* last_cell = nullptr;
        size_t index = 0;
        
        Field field;
        [[no_unique_address]] SwitchStateFunc switch_func;

        SFMLVisualizer(sf::Color fill = sf::Color::Black, sf::Color outline = sf::Color::White) {
            float current_x = 0.f;
            float current_y = 0.f;
            for(auto& row: field.cells){
                for(auto& cell: row){
                    cell.rs.setSize(sf::Vector2f(cell_size, cell_size));
                    cell.rs.setPosition(current_x, current_y);
                    cell.rs.setFillColor(fill);
                    cell.rs.setOutlineThickness(1.f);
                    cell.rs.setOutlineColor(outline);
                    current_x += cell_size;
                }
                current_x = 0;
                current_y += cell_size;
            }
        }

        Cell* get_cell_by_pos(float x, float y) {
            if (last_cell && last_cell->rs.getGlobalBounds().contains(x, y)) {
                return last_cell;
            }
            auto c_cell = std::find_if(cache.begin(), cache.end(), [x, y](auto value){
                return value != nullptr ? value->rs.getGlobalBounds().contains(x, y) : false;
            });

            if (c_cell != cache.end()) {
                last_cell = *c_cell;
                return last_cell;
            } else {
                for(auto& row: field.cells) {
                    auto res = std::find_if(std::begin(row), std::end(row), [x, y](auto& cell){
                        return cell.rs.getGlobalBounds().contains(x, y);
                    });
                    if (res != std::end(row)) {
                        index %= cache.size();
                        cache[index++] = res;
                        last_cell = res;
                        return res;
                    }
                }
            } 
            return nullptr;
        }

        void display() {
            sf::RenderWindow window(sf::VideoMode(field.get_width()*cell_size, field.get_height()*cell_size), "Automaton");
            sf::RectangleShape background(sf::Vector2f(window.getSize().x, window.getSize().y));
            background.setFillColor(BACKGROUND_COLOR);

            window.setFramerateLimit(50);

            bool running = false;
            auto state_to_set = field.get_default_state();
            size_t last_fps = 1;

            while(window.isOpen()) {
            sf::Event event;
            while(window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                if (event.type == sf::Event::KeyReleased) {
                    if (event.key.code == sf::Keyboard::Num1) {
                        window.setFramerateLimit(1);
                        last_fps = 1;
                    } else if (event.key.code == sf::Keyboard::Num2) {
                        window.setFramerateLimit(10);
                        last_fps = 10;
                    } else if (event.key.code == sf::Keyboard::Num3) {
                        window.setFramerateLimit(60);
                        last_fps = 60;
                    } else if (event.key.code == sf::Keyboard::P) {
                        running = running ? (window.setFramerateLimit(100), false) : (window.setFramerateLimit(last_fps), true);
                    } else if (event.key.code == sf::Keyboard::F) {
                        field.flush();
                    } else if (event.key.code == sf::Keyboard::M) {
                        state_to_set = switch_func(state_to_set);
                    }
                }
            }

            if (running){
                field.next_gen();
            }

            if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && window.hasFocus()){
                auto m_pos = sf::Mouse::getPosition(window);
                if (background.getGlobalBounds().contains(m_pos.x, m_pos.y)) {
                    auto cell = get_cell_by_pos(m_pos.x, m_pos.y);
                    if (cell) {
                        cell->set_state(state_to_set);
                    }
                }
            }

            window.clear();
            window.draw(background);
            
            for(auto& row: field.cells)
                for(auto& cell: row)
                    window.draw(cell.rs);

            window.display();
           }
         }
        };
};
#endif