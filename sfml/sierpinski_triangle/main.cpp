#include <iostream>
#include <SFML/Graphics.hpp>


int main() {
    sf::RenderWindow window(sf::VideoMode(1024, 800), "SierpinskiTriangle");
    sf::RectangleShape background(sf::Vector2f(window.getSize().x, window.getSize().y));
    background.setFillColor(sf::Color::White);

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
    current_number_text.setFillColor(sf::Color::Black);

    while(window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        window.clear();
        window.draw(background);
        window.draw(current_number_text);
        window.display();
    }
}