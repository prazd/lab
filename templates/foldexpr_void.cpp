#include <iostream>

// from patterns/facade (for example)

struct VerifierF {bool operator()(int x){return x > 0;}};
struct VerifierS {bool operator()(int x){return x % 2 == 0;}};
struct VerifierT {bool operator()(int x){return x < std::numeric_limits<int>::max();}};

struct System {
    enum class Level: char {first, second, third};
    
    static void run(const std::initializer_list<Level>& lvls, int value) {
        for(auto lvl: lvls)
            if (!verify(lvl, value)) throw std::invalid_argument("Invalid value");
        std::cout << "Run...." << "\n";
    }

    static void run(int value, auto&&... args) {
        (void(!verify(args, value) ? throw std::invalid_argument("Invalid value") : void()), ...); // example
        std::cout << "Run...." << "\n";
    }

    static bool verify(Level lvl, int value) {
        switch (lvl) {
            case Level::first:  {return VerifierF{}(value);}
            case Level::second: {return VerifierS{}(value);}
            case Level::third:  {return VerifierT{}(value);}
            default: {
                throw std::invalid_argument("Invalid lvl");
            }
        }
    }
};

int main() {
    System::run(100, System::Level::first);
    System::run(44, System::Level::first, System::Level::second);
}
