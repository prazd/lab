#include <vector>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Mouse.hpp>
#include <thread>
#include <algorithm>

constexpr float cell_size = 10.f;

class Cell {
private:
    bool alive;
    int alive_neighbors;
    //      leftUp    | up   | rightUp
    //      left      | CELL | right
    //      leftDown  | down | rightDown
public:
    sf::RectangleShape rs;
    Cell* neighbours[8];
    void set_is_alive(bool s);
    bool is_alive();
    void count_alive_neighbors();
    void check_for_life();
};


template <size_t height, size_t width>
class BoardStatic {
public:    
    static inline Cell cells[height][width];
    static inline std::array<Cell*, 20> cache;
    Cell* last_cell = nullptr;
    size_t index = 0;

    BoardStatic() {
        seed();
    }

    void seed() {
        
        float current_x = 0.f;
        float current_y = 0.f;

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                // [ [ 0 § 0 0 0 0 0 ] ]
                // [ [ 0 0 0 0 0 0 0 ] ]
                // [ [ 0 0 0 0 0 0 0 ] ]

                cells[i][j].rs.setSize(sf::Vector2f(10, 10));
                cells[i][j].rs.setPosition(current_x, current_y);
                cells[i][j].rs.setFillColor(sf::Color::Black);
                cells[i][j].rs.setOutlineThickness(1.f);
                cells[i][j].rs.setOutlineColor(sf::Color::White);

                current_x += cell_size;
                cells[i][j].neighbours[0] = &cells[i][(j - 1 + width) % width];
                cells[i][j].neighbours[1] = &cells[i][(j + 1) % width];
                cells[i][j].neighbours[2] = &cells[(i - 1 + height) % height][j];
                cells[i][j].neighbours[3] = &cells[(i + 1) % height][j];

                cells[i][j].neighbours[4] = &cells[(i - 1 + height) % height][(j - 1 + width) % width];
                cells[i][j].neighbours[5] = &cells[(i - 1 + height) % height][(j + 1) % width];
                cells[i][j].neighbours[6] = &cells[(i + 1) % height][(j - 1 + width) % width];
                cells[i][j].neighbours[7] = &cells[(i + 1) % height][(j + 1) % width];
            }
            current_x = 0;
            current_y += cell_size;
        }
    }
    

    Cell* getCellByPos(float x, float y) {
        // cache
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
            for(auto l: cells) {
                auto res = std::find_if(l, l+width, [x, y](auto& cell){
                    return cell.rs.getGlobalBounds().contains(x,y);
                });
                if (res != l+width) {
                    index %= cache.size();
                    cache[index++] = res;
                    last_cell = res;
                    return res;
                }
            }
        } 
        return nullptr;
    }

    void update_state(int hIndex, int wIndex, bool state) {
        if (hIndex >= 0 && hIndex < height && wIndex >= 0 && wIndex < width){
            cells[hIndex][wIndex].set_is_alive(state);
        } else {
            std::cout << "update_state failed" << std::endl;
        }
    }

    void next_gen() {
        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                cells[i][j].count_alive_neighbors();
            }
        }

        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                cells[i][j].check_for_life();
            }
        }
    }

    void flush() {
        for(auto l: cells) {
            for(auto it = l, ite = l+width;it!=ite;++it) {
                it->set_is_alive(false);
            }
        }
    }

    constexpr size_t get_height() const {
        return height;
    }

    constexpr size_t get_width() const {
        return width;
    }
};

void Cell::set_is_alive(bool s) {
    alive = s;
    if (alive) {
        rs.setFillColor(sf::Color::White);
    } else {
        rs.setFillColor(sf::Color::Black);
    }
}
bool Cell::is_alive() {
    return alive;
}
void Cell::count_alive_neighbors() {
    int res{};
    for (auto el: neighbours) {
        if (el != nullptr && el->is_alive()){
             ++res;
        }
    }
    alive_neighbors = res;
}

void Cell::check_for_life() {
    // в пустой (мёртвой) клетке, с которой соседствуют три живые клетки, зарождается жизнь;
    // если у живой клетки есть две или три живые соседки, то эта клетка продолжает жить; в противном случае (если живых соседей меньше двух или больше трёх) клетка умирает («от одиночества» или «от перенаселённости»)
    if (!alive && alive_neighbors == 3) {
        set_is_alive(true);
        return;
    }
    if (alive && (alive_neighbors == 3 || alive_neighbors == 2)) {
        return;
    }
    if (alive && (alive_neighbors < 2 || alive_neighbors > 3)){
        set_is_alive(false);
        return;
    }
}


template <size_t height, size_t width>
class Game {
public:
    BoardStatic<height, width> board;

    void glider_gun() {
        board.update_state(5, 1, true);
        board.update_state(5, 2, true);
        board.update_state(6, 1, true);
        board.update_state(6, 2, true);

        board.update_state(3, 13, true);
        board.update_state(3, 14, true);
        board.update_state(4, 12, true);
        board.update_state(5, 11, true);
        board.update_state(6, 11, true);
        board.update_state(7, 11, true);
        board.update_state(8, 12, true);
        board.update_state(9, 13, true);
        board.update_state(9, 14, true);

        board.update_state(6, 15, true);
        board.update_state(4, 16, true);
        board.update_state(8, 16, true);
        board.update_state(5, 17, true);
        board.update_state(6, 17, true);
        board.update_state(7, 17, true);
        board.update_state(6, 18, true);

        board.update_state(3, 21, true);
        board.update_state(4, 21, true);
        board.update_state(5, 21, true);
        board.update_state(3, 22, true);
        board.update_state(4, 22, true);
        board.update_state(5, 22, true);
        board.update_state(2, 23, true);
        board.update_state(6, 23, true);

        board.update_state(1, 25, true);
        board.update_state(2, 25, true);
        board.update_state(6, 25, true);
        board.update_state(7, 25, true);

        board.update_state(3, 35, true);
        board.update_state(3, 36, true);
        board.update_state(4, 35, true);
        board.update_state(4, 36, true);
    }
};

const auto BACKGROUND_COLOR = sf::Color::White;

void graphic() {
    constexpr size_t height = 80;
    constexpr size_t width = 150;
    Game<height, width> g;

    sf::RenderWindow window(sf::VideoMode(width*cell_size, height*cell_size), "GoL");
    sf::RectangleShape background(sf::Vector2f(window.getSize().x, window.getSize().y));
    background.setFillColor(BACKGROUND_COLOR);

    window.setFramerateLimit(50);

    bool running = false;
    bool set_alive = true;

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
                    running = running ? (window.setFramerateLimit(60), false) : (window.setFramerateLimit(last_fps), true);
                } else if (event.key.code == sf::Keyboard::F) {
                   g.board.flush();  
                } else if (event.key.code == sf::Keyboard::M) {
                   set_alive = set_alive ? false : true;
                }
            }
        }

        if (running){
            g.board.next_gen();         
        }

        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)){
            auto m_pos = sf::Mouse::getPosition(window);
            if (background.getGlobalBounds().contains(m_pos.x, m_pos.y)) {
                auto cell = g.board.getCellByPos(m_pos.x, m_pos.y);
                if (cell) {
                    cell->set_is_alive(set_alive);
                }
            }
        }

        window.clear();
        window.draw(background);
        for(int i = 0;i < g.board.get_height(); ++i){
            for(int j = 0; j < g.board.get_width(); ++j) {
                window.draw(g.board.cells[i][j].rs);
            }
        }
        window.display();
    }
}

int main() {
    graphic();
    return 0;
}