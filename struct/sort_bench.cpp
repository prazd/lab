#include <iostream>
#include <algorithm>
#include <iomanip>
#include <random>
#include <numeric>
#include <time.h>

bool sort_predicat(int f, int s) {
    return f < s;
}

struct Predicat {
    bool operator()(int f, int s) {return f < s;}
};

void benchmark(char* elcount) {
    std::string c(elcount);
    auto elems = std::stoi(c);
    int *arr = new int[elems];
    std::cout << "Contructing random values vector (size = " << elcount << ")" << std::endl;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, elems);

    for(int i = 0;i<elems;++i) {
        arr[i] = distrib(gen);
    }
    
    auto start = clock();
    std::cout << "Start sorting..."<< std::endl;
    std::sort(arr, arr+elems,Predicat{});
    auto end = clock();
    
    auto elapsed_time = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    std::cout << std::fixed << std::setprecision(2) << elapsed_time   << "s" << std::endl;

    delete[] arr;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return -1;
    }
    benchmark(argv[1]);
    return 0;
}