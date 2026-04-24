#include <iostream>
#include <cassert>

struct M {
    enum S {A1, A2, A3, S_SZ};
    enum I {Current, Next, I_SZ};

    inline static constexpr S next_table[S_SZ][I_SZ]{{A1,A2},{A2,A3},{A3,A1}};
    inline static constexpr void(*handlers[S_SZ][I_SZ])(){
        {[]{std::cout << "A1\n";}, []{std::cout << "A1->A2\n";}},
        {[]{std::cout << "A2\n";}, []{std::cout << "A2->A3\n";}},
        {[]{std::cout << "A3\n";}, []{std::cout << "A3->A1\n";}}
    };

    S state = A1;
    void step(I input){
        handlers[state][input]();
        state = next_table[state][input];
    }
};

int main(){
    M m;
    assert(m.state == M::S::A1);
    
    m.step(M::Next);
    assert(m.state == M::S::A2);
    
    m.step(M::Current);
    assert(m.state == M::S::A2);

    m.step(M::Next);
    assert(m.state == M::S::A3);

    m.step(M::Current);
    assert(m.state == M::S::A3);

    m.step(M::Next);
    assert(m.state == M::S::A1);
}