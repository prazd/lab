#include <iostream>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <random>
#include <sstream>

const auto P1_COLOR = sf::Color::Blue;
const auto P2_COLOR = sf::Color::Red;
const auto BACKGROUND_COLOR = sf::Color::Black;
constexpr size_t round_counts = 1000'000;
constexpr size_t scale = 150;
constexpr bool graphic_mode = false;

class MontyHall {
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<int> dis = std::uniform_int_distribution<int>(0,2);

    bool doors[3]{false, false, true};

    size_t p1_wins{};
    size_t p2_wins{};

    const auto& get_doors() const&  {
        return doors;
    }

    int roll() {
        return dis(gen);
    }

public:
    MontyHall(): gen(rd()){}

    size_t get_p1_wins() const {
        return p1_wins;
    }
    size_t get_p2_wins() const {
        return p2_wins;
    }

    void round() {
        std::shuffle(std::begin(doors), std::end(doors), gen);
        auto p1 = roll();
        auto p2 = roll();

        auto win_addr = std::find_if(std::begin(doors), std::end(doors), [](const auto el){return el == 1;});
        auto p1_addr = doors+p1;
        auto p2_addr = doors+p2;

        if (p2_addr != win_addr){
            p2_addr = win_addr;
        } else {
            if (p2_addr == doors) {
                p2_addr = doors+1;
            } else if (p2_addr == doors+1) {
                p2_addr = doors+2;
            } else {
                p2_addr = doors;
            }
        }

        if (p1_addr == win_addr) {
            ++p1_wins;
        }
        if (p2_addr == win_addr) {
            ++p2_wins;
        }
    }
};

template <typename... Args>
void update_sf_text(std::stringstream& ss, sf::Text& t, Args... args) {
    (ss << ... << args);
    t.setString(ss.str());
    ss.str("");
    ss.clear();
}


void sfml_sim() {
    sf::RenderWindow window(sf::VideoMode(1024, 800), "MontyHall");
    sf::RectangleShape background(sf::Vector2f(window.getSize().x, window.getSize().y));
    background.setFillColor(BACKGROUND_COLOR);
    
    sf::Font font;
    const std::string font_path = "/Users/prazd/Desktop/books/cpp/TimberSFML/fonts/KOMIKAP_.ttf";
    if (!font.loadFromFile(font_path)) {
        std::cerr << "Error loading font" << std::endl;
        std::terminate();
    }
    
    MontyHall m;

    sf::RectangleShape p1(sf::Vector2f(100.f, 10.f));
    p1.setFillColor(P1_COLOR);
    p1.setPosition(300.f, 600.f);

    sf::RectangleShape p2(sf::Vector2f(100.f, 10.f));
    p2.setFillColor(P2_COLOR);
    p2.setPosition(600.f, 600.f);

    sf::Text p1_wins_text;
    p1_wins_text.setFont(font);
    p1_wins_text.setCharacterSize(24);
    p1_wins_text.setPosition(300.f, 650.f);
    p1_wins_text.setFillColor(sf::Color::White);

    sf::Text p2_wins_text;
    p2_wins_text.setFont(font);
    p2_wins_text.setCharacterSize(24);
    p2_wins_text.setPosition(600.f, 650.f);
    p2_wins_text.setFillColor(sf::Color::White);

    sf::Text counter_text;
    counter_text.setFont(font);
    counter_text.setCharacterSize(24);
    counter_text.setPosition(window.getSize().x/2, 0.f);
    counter_text.setFillColor(sf::Color::White);

    std::stringstream ss;

    size_t counter = 0;
    while(window.isOpen()) {
        sf::Event event;
        while(window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        window.clear();
        
        if (counter <= round_counts) {
            m.round();
            p1.setSize(sf::Vector2f(100.f, static_cast<float>(m.get_p1_wins()/scale) * -1));
            p2.setSize(sf::Vector2f(100.f, static_cast<float>(m.get_p2_wins()/scale) * -1));

            update_sf_text(ss, p1_wins_text, "P1:", m.get_p1_wins());
            update_sf_text(ss, p2_wins_text, "P2:", m.get_p2_wins());
            update_sf_text(ss, counter_text, "Round:", counter);
            
            ++counter;
        }
        window.draw(p1_wins_text);
        window.draw(p2_wins_text);
        window.draw(counter_text);

        window.draw(p1);
        window.draw(p2);

        window.display();
    }
}

void simple_sim() {
    MontyHall m;
    for(int i = 0; i < round_counts; ++i) {
        m.round();
    }
    std::cout << "P1 wins:" << m.get_p1_wins() << std::endl;
    std::cout << "P2 wins:" << m.get_p2_wins() << std::endl;
}

int main() {
    if constexpr(graphic_mode) {
        sfml_sim();
    } else {
        simple_sim();
    }
}