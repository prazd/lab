#include <iostream>

int main() {
    std::vector<int> lvec{1,2,3,4,5};

    // Для перемещения объектов в замыкания используется инициализирующий захват С++14.
    auto l = [vec = std::move(lvec)](){
        for(auto el: vec) {
            std::cout << el;
        }
        std::cout << std::endl;
    };
    
    // c++11
    std::vector<int> bvec{6,7,8,9,10};
    
    // В С++11 инициализирующий захват эмулируется с помощью написания классов вручную или применения std::bind;
    auto bl = std::bind([](std::vector<int>& vec){
        for(auto el: vec) {
            std::cout << el;
        }
        std::cout << std::endl;
    }, std::move(bvec));

    l();
    bl();
}