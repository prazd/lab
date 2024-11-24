#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <random>
#include <sstream>
#include <chrono>
#include <thread>
#include <numeric>

const auto POINT_COLOR = sf::Color::White;
const auto BACKGROUND_COLOR = sf::Color::Black;
constexpr size_t ANGLES = 3;
constexpr float CIRCLE_RADIUS = 2.f;
const auto PointerFunc = [](float x, float y){return std::midpoint(x,y);};
// const auto PointerFunc = [](float x, float y){return x + (y - x)/3;};

template <int begin, int end>
class RandomCube {
    static inline std::random_device rd;
    static inline std::mt19937 gen{rd()};
    static inline std::uniform_int_distribution<int> dis = std::uniform_int_distribution<int>(begin, end);
public:
    static int roll() {
        return dis(gen);
    }
};

template <size_t points, typename PointFunc>
class ShapeGenerator {
    std::array<sf::CircleShape, points> shapes;
    static inline std::map<size_t, size_t> mindex;
    
    [[no_unique_address]] PointFunc pfunc;

    int current_num;
public:
    ShapeGenerator(const std::initializer_list<sf::CircleShape>& sparams){
        std::copy(sparams.begin(), sparams.end(), shapes.begin());
        size_t cindex = 1;
        for(size_t i = 0;i<shapes.size(); ++i){
            mindex[cindex++] = i;
            mindex[cindex++] = i;
        }
    }

    ShapeGenerator(const std::array<sf::CircleShape, points>& shapes): shapes(shapes) {
        size_t cindex = 1;
        for(size_t i = 0;i<shapes.size(); ++i){
            mindex[cindex++] = i;
            mindex[cindex++] = i;
        }
    }

    const sf::CircleShape* get_shape(int num) {
        return &shapes[mindex[num]];
    }

    sf::CircleShape gen(const sf::CircleShape& last) {
        sf::CircleShape result(CIRCLE_RADIUS);
        result.setFillColor(POINT_COLOR);

        const sf::CircleShape* cp = nullptr;
        
        current_num = RandomCube<1, points*2>::roll();

        cp = get_shape(current_num);

        result.setPosition(
            pfunc(cp->getPosition().x, last.getPosition().x),
            pfunc(cp->getPosition().y, last.getPosition().y)
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


// GPT
template <size_t n>
std::array<sf::CircleShape, n> generatePolygonVertices(
    float radius, // Радиус окружности
    float centerX, // Центр по X
    float centerY, // Центр по Y
    float pointRadius = 2.f // Радиус визуальной точки
) {
    if (n < 3) {
        throw std::invalid_argument("A polygon must have at least 3 sides.");
    }

    std::array<sf::CircleShape, n> vertices;
    const float angleStep = 2 * M_PI / n; // Угол между вершинами

    for (int i = 0; i < n; ++i) {
        float angle = i * angleStep; // Текущий угол
        float x = centerX + radius * std::cos(angle);
        float y = centerY + radius * std::sin(angle);

        sf::CircleShape point(pointRadius);
        point.setPosition(x, y);
        point.setFillColor(POINT_COLOR);

        vertices[i] = point;
    }

    return vertices;
}

int main() {
    sf::RenderWindow window(sf::VideoMode(1024, 800), "SierpinskiTriangle");
    sf::RectangleShape background(sf::Vector2f(window.getSize().x, window.getSize().y));
    background.setFillColor(BACKGROUND_COLOR);

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
    cube_number_text.setFillColor(POINT_COLOR);

    sf::Text point_count_text;
    point_count_text.setFont(font);
    point_count_text.setCharacterSize(24);
    point_count_text.setPosition(window.getSize().x - 300.f, 0.f);
    point_count_text.setFillColor(POINT_COLOR);

    sf::Text mode_text;
    mode_text.setFont(font);
    mode_text.setCharacterSize(24);
    mode_text.setPosition(window.getSize().x - 600.f, 0.f);
    mode_text.setFillColor(POINT_COLOR);

    sf::Text fps_text;
    fps_text.setFont(font);
    fps_text.setCharacterSize(24);
    fps_text.setFillColor(POINT_COLOR);
    fps_text.setPosition(window.getSize().x/2, window.getSize().y/2 + 200.f);

    auto points = generatePolygonVertices<ANGLES>(200.f, 400.f, 400.f);
    
    // init point
    sf::CircleShape init_point(CIRCLE_RADIUS);
    init_point.setFillColor(POINT_COLOR);
    init_point.setPosition(window.getSize().x / 2, (window.getSize().y / 2)-50.f);
    
    std::vector<sf::CircleShape> vec_shapes{points.begin(), points.end()};
    vec_shapes.push_back(init_point);

    ShapeGenerator<ANGLES, decltype(PointerFunc)> sg{points};

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

        for(const auto& el: vec_shapes)
            window.draw(el);
       
        window.display();
    }
}