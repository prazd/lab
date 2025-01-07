#include <iostream>

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
    static bool verify(Level lvl, int value) {
        switch (lvl) {
            case Level::first:  {return VerifierF{}(value);}
            case Level::second: {return VerifierS{}(value);}
            case Level::third:  {return VerifierT{}(value);}
        }
    }
};

int main() {
    System::run({System::Level::first}, 100);
    System::run({System::Level::first, System::Level::second}, 44);
}