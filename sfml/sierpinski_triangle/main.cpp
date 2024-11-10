#include <iostream>
#include <SFML/Graphics.hpp>
#include <random>
#include <sstream>
#include <chrono>
#include <thread>
#include <numeric>

class Random6Cube {
    static inline std::random_device rd;
    static inline std::mt19937 gen;
    static inline std::uniform_int_distribution<int> dis = std::uniform_int_distribution<int>(1,6);
public:
    static int get() {
        return dis(gen);
    }
};

class ShapeGenerator {
    sf::CircleShape p1;
    sf::CircleShape p2;
    sf::CircleShape p3;
    int current_num;
public:
    ShapeGenerator(const sf::CircleShape& p1, const sf::CircleShape& p2, const sf::CircleShape& p3): 
    p1(p1), p2(p2), p3(p3){}
    sf::CircleShape gen(const sf::CircleShape& last) {
        current_num = Random6Cube::get();
        sf::CircleShape result(2.f);
        result.setFillColor(sf::Color::Red);
        if (current_num == 1 || current_num == 2) {
            result.setPosition(std::midpoint(p1.getPosition().x, last.getPosition().x),std::midpoint(p1.getPosition().y, last.getPosition().y));
        } else if (current_num == 3 || current_num == 4) {
            result.setPosition(std::midpoint(p2.getPosition().x, last.getPosition().x),std::midpoint(p2.getPosition().y, last.getPosition().y));
        } else {
            result.setPosition(std::midpoint(p3.getPosition().x, last.getPosition().x),std::midpoint(p3.getPosition().y, last.getPosition().y));
        }
        return result;
    }
    int get_current_num() const {
        return current_num;
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(1024, 800), "SierpinskiTriangle");
    sf::RectangleShape background(sf::Vector2f(window.getSize().x, window.getSize().y));
    background.setFillColor(sf::Color::Black);

    sf::Font font;
    const std::string font_path = "/Users/prazd/Desktop/books/cpp/TimberSFML/fonts/";
    if (!font.loadFromFile(font_path + "KOMIKAP_.ttf")) {
        std::cerr << "Error loading font" << std::endl;
        std::terminate();
    }

    sf::Text current_number_text;
    current_number_text.setFont(font);
    current_number_text.setCharacterSize(24);
    current_number_text.setString("Current: 0");
    current_number_text.setPosition(0.f, 0.f);
    current_number_text.setFillColor(sf::Color::Red);

    Random6Cube rc;

    // triangle
    sf::CircleShape p1(2.f);
    p1.setFillColor(sf::Color::Red);
    p1.setPosition(window.getSize().x/2, 50.f);

    sf::CircleShape p2(2.f);
    p2.setFillColor(sf::Color::Red);
    p2.setPosition(50.f, (window.getSize().y/2)+100.f);

    sf::CircleShape p3(2.f);
    p3.setFillColor(sf::Color::Red);
    p3.setPosition((window.getSize().x)-50.f, (window.getSize().y/2)+100.f);

    // 
    sf::CircleShape p4(2.f);
    p4.setFillColor(sf::Color::Red);
    p4.setPosition(p1.getPosition().x + 200.f, p2.getPosition().y - 40.f);

    std::vector<sf::CircleShape> vec_shapes{p1,p2,p3,p4};

    ShapeGenerator sg(p1,p2,p3);

    std::stringstream ss;
    ss << "Current: 0";
    current_number_text.setString(ss.str());
    ss.str("");
    ss.clear();


    while(window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        window.clear();
       
        window.draw(background);
        
        auto new_shape = sg.gen(vec_shapes.back());
        vec_shapes.push_back(new_shape);
        ss << "Current:" << sg.get_current_num();
        current_number_text.setString(ss.str());
        ss.str("");
        ss.clear();

        for(auto& el: vec_shapes)
            window.draw(el);

        window.draw(current_number_text);
        std::cout << "Vec size:" << vec_shapes.size() << std::endl;
        window.display();
    }
}