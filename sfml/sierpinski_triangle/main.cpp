#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <random>
#include <sstream>
#include <chrono>
#include <thread>
#include <numeric>

auto PointColor = sf::Color::Red;
auto BackgroundColor = sf::Color::Black;

class Random6Cube {
    static inline std::random_device rd;
    static inline std::mt19937 gen{rd()};
    static inline std::uniform_int_distribution<int> dis = std::uniform_int_distribution<int>(1,6);
public:
    static int roll() {
        return dis(gen);
    }
};

class ShapeGenerator {
    const sf::CircleShape p1;
    const sf::CircleShape p2;
    const sf::CircleShape p3;

    int current_num;
public:
    ShapeGenerator(const sf::CircleShape& p1, const sf::CircleShape& p2, const sf::CircleShape& p3): p1(p1), p2(p2), p3(p3){}

    sf::CircleShape gen(const sf::CircleShape& last) {
        sf::CircleShape result(2.f);
        result.setFillColor(PointColor);

        const sf::CircleShape* cp = nullptr;
        
        current_num = Random6Cube::roll();
        if (current_num == 1 || current_num == 2) {
            cp = &p1;
        } else if (current_num == 3 || current_num == 4) {
            cp = &p2;
        } else {
            cp = &p3;
        }

        result.setPosition(
            std::midpoint(cp->getPosition().x, last.getPosition().x), 
            std::midpoint(cp->getPosition().y, last.getPosition().y)
        );
        return result;
    }
    int get_current_num() const {
        return current_num;
    }
};

template <typename... Args>
void update_sf_text(std::stringstream& ss, sf::Text& t, Args... args) {
    (ss << ... << args);
    t.setString(ss.str());
    ss.str("");
    ss.clear();
}

int main() {
    sf::RenderWindow window(sf::VideoMode(1024, 800), "SierpinskiTriangle");
    sf::RectangleShape background(sf::Vector2f(window.getSize().x, window.getSize().y));
    background.setFillColor(BackgroundColor);

    sf::Font font;
    const std::string font_path = "/Users/prazd/Desktop/books/cpp/TimberSFML/fonts/KOMIKAP_.ttf";
    if (!font.loadFromFile(font_path)) {
        std::cerr << "Error loading font" << std::endl;
        std::terminate();
    }

    sf::Text cube_number_text;
    cube_number_text.setFont(font);
    cube_number_text.setCharacterSize(24);
    cube_number_text.setString("Cube: 0");
    cube_number_text.setPosition(0.f, 0.f);
    cube_number_text.setFillColor(PointColor);

    sf::Text point_count_text;
    point_count_text.setFont(font);
    point_count_text.setCharacterSize(24);
    point_count_text.setPosition(window.getSize().x - 300.f, 0.f);
    point_count_text.setFillColor(PointColor);

    sf::Text mode_text;
    mode_text.setFont(font);
    mode_text.setCharacterSize(24);
    mode_text.setPosition(window.getSize().x - 600.f, 0.f);
    mode_text.setFillColor(PointColor);

    sf::Text fps_text;
    fps_text.setFont(font);
    fps_text.setCharacterSize(24);
    fps_text.setFillColor(PointColor);
    fps_text.setPosition(window.getSize().x/2, window.getSize().y/2 + 200.f);
    

    // triangle
    sf::CircleShape p1(2.f);
    p1.setFillColor(PointColor);
    p1.setPosition(window.getSize().x/2, 50.f);

    sf::CircleShape p2(2.f);
    p2.setFillColor(PointColor);
    p2.setPosition(50.f, (window.getSize().y/2)+100.f);

    sf::CircleShape p3(2.f);
    p3.setFillColor(PointColor);
    p3.setPosition((window.getSize().x)-50.f, (window.getSize().y/2)+100.f);

    // init point
    sf::CircleShape init_point(2.f);
    init_point.setFillColor(PointColor);
    init_point.setPosition(window.getSize().x / 2, window.getSize().y / 2);

    std::vector<sf::CircleShape> vec_shapes{p1,p2,p3,init_point};
    ShapeGenerator sg(p1,p2,p3);
    std::stringstream ss;

    // init mode
    window.setFramerateLimit(10);
    update_sf_text(ss, mode_text, "Mode: Slow");

    sf::Clock clock;
    while(window.isOpen()) {
        sf::Time elapsed = clock.restart();
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyReleased) {
                if (event.key.code == sf::Keyboard::Num1) {
                    window.setFramerateLimit(10);
                    update_sf_text(ss, mode_text, "Mode: Slow");
                } else if (event.key.code == sf::Keyboard::Num2) {
                    window.setFramerateLimit(100);
                    update_sf_text(ss, mode_text, "Mode: Medium");
                } else if (event.key.code == sf::Keyboard::Num3) {
                    window.setFramerateLimit(300);
                    update_sf_text(ss, mode_text, "Mode: Fast");
                }
            }
        }
        
        auto new_shape = sg.gen(vec_shapes.back());
        vec_shapes.push_back(new_shape);
     
        update_sf_text(ss, cube_number_text, "Cube:", sg.get_current_num());
        update_sf_text(ss, point_count_text, "Points:", vec_shapes.size());
        update_sf_text(ss, fps_text, "FPS:", static_cast<int>(1.0f / elapsed.asSeconds()));

        window.clear();
        window.draw(background);    
        window.draw(cube_number_text);
        window.draw(point_count_text);
        window.draw(mode_text);
        window.draw(fps_text);

        for(auto& el: vec_shapes)
            window.draw(el);
       
        window.display();
    }
}