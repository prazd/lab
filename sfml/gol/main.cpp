#include <vector>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <thread>

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


template <size_t height, size_t width, bool infinite>
class BoardStatic {
public:    
    static inline Cell cells[height][width];

    constexpr BoardStatic() {
        if constexpr (infinite) {
            seed_infinite();
        }
    }
    constexpr void seed_infinite() {
        float current_x = 0.f;
        float current_y = 0.f;
        // auto current_color = sf::Color::Green;
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

                current_x += 10.f;
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
            current_y += 10.f;
        }
    }

    // constexpr void seed() {
    //     for (int i = 0; i < height; i++) {
    //         for (int j = 0; j < width; j++) {
    //             // [ [ 0 § 0 0 0 0 0 ] ]
    //             // [ [ 0 0 0 0 0 0 0 ] ]
    //             // [ [ 0 0 0 0 0 0 0 ] ]

    //             cells[i][j].neighbours[0] = j > 0 ? &cells[i][j - 1] : nullptr;
    //             cells[i][j].neighbours[1] = j < width - 1 ? &cells[i][j + 1] : nullptr;
    //             cells[i][j].neighbours[2] = i > 0 ? &cells[i - 1][j] : nullptr;
    //             cells[i][j].neighbours[3] = i < height - 1 ? &cells[i + 1][j] : nullptr;


    //             cells[i][j].neighbours[4] = i > 0 && j > 0 ? &cells[i - 1][j - 1] : nullptr;
    //             cells[i][j].neighbours[5] = i > 0 && j < width - 1 ? &cells[i - 1][j + 1] : nullptr;
    //             cells[i][j].neighbours[6] = i < height - 1 && j > 0 ? &cells[i + 1][j - 1] : nullptr;
    //             cells[i][j].neighbours[7] = i < height - 1 && j < width - 1 ? &cells[i + 1][j + 1] : nullptr;
    //         }
    //     }
    // }

    int count_alive_cells() {
        int counter = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (cells[i][j].is_alive()) {
                    ++counter;
                }
            }
        }
        return counter;
    }

    void update_state(int hIndex, int wIndex, bool state) {
        if (hIndex >= 0 && hIndex <= height && wIndex >= 0 && wIndex <= width){
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
        alive = true;
        rs.setFillColor(sf::Color::White);
        return;
    }
    if (alive && (alive_neighbors == 3 || alive_neighbors == 2)) {
        return;
    }
    if (alive && (alive_neighbors < 2 || alive_neighbors > 3)){
        rs.setFillColor(sf::Color::Black);
        alive = false;
        return;
    }
}


template <size_t height, size_t width, bool infinite>
class Game {
public:
    BoardStatic<height, width, infinite> board;

    void glider() {
        board.update_state(1, 2, true);
        board.update_state(2, 3, true);
        board.update_state(3, 1, true);
        board.update_state(3, 2, true);
        board.update_state(3, 3, true);
    }
    void glider_gun() {
        // Устанавливаем начальное состояние для глайдер-пушки
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

    void chaos() {
        board.update_state(1, 36, true);
        board.update_state(2, 36, true);
        board.update_state(3, 36, true);
        board.update_state(4, 36, true);
        board.update_state(5, 36, true);
        board.update_state(6, 36, true);
        board.update_state(7, 36, true);
        board.update_state(8, 36, true);
        board.update_state(9, 36, true);
        board.update_state(10, 36, true);

        board.update_state(1, 24, true);
        board.update_state(2, 24, true);
        board.update_state(3, 24, true);
        board.update_state(4, 24, true);
        board.update_state(5, 24, true);
        board.update_state(6, 24, true);
        board.update_state(7, 24, true);
        board.update_state(8, 24, true);
        board.update_state(9, 24, true);
        board.update_state(10, 24, true);

        board.update_state(3, 12, true);
        board.update_state(4, 12, true);
        board.update_state(5, 12, true);
        board.update_state(6, 12, true);
        board.update_state(7, 12, true);
        board.update_state(8, 12, true);
        board.update_state(9, 12, true);
        board.update_state(10, 12, true);

        board.update_state(5, 0, true);
        board.update_state(6, 0, true);
        board.update_state(7, 0, true);
        board.update_state(8, 0, true);
        board.update_state(9, 0, true);
        board.update_state(10, 0, true);

        board.update_state(1, 22, true);
        board.update_state(2, 22, true);
        board.update_state(6, 22, true);
        board.update_state(7, 22, true);

        board.update_state(2, 12, true);
        board.update_state(6, 12, true);

        board.update_state(3, 13, true);
        board.update_state(4, 13, true);
        board.update_state(5, 13, true);
        board.update_state(6, 13, true);
        board.update_state(7, 13, true);
    }


    void next_gen() {
        board.next_gen();
    }

    void start() {
        chaos();
        std::cout << "\x1B[2J\x1B[H";
        while(board.count_alive_cells() != 0){
            board.next_gen();
            board.print();
            std::this_thread::sleep_for(std::chrono::milliseconds (60));
        }
        std::cout << "------------------------\n";
        std::cout << "All cells is dead";
    }
};

const auto POINT_COLOR = sf::Color::Red;
const auto BACKGROUND_COLOR = sf::Color::White;

void graphic() {
    sf::RenderWindow window(sf::VideoMode(1024, 800), "GoL");
    sf::RectangleShape background(sf::Vector2f(window.getSize().x, window.getSize().y));
    background.setFillColor(BACKGROUND_COLOR);

    Game<80, 150, true> g;
    g.glider_gun();
    while(window.isOpen()) {
        sf::Event event;
        while(window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        g.next_gen();
        window.clear();
        window.draw(background);
        for(int i = 0;i < 80;++i){
            for(int j = 0; j < 150; ++j) {
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