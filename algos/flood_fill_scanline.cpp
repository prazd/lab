#include <iostream>
#include <stack>

constexpr size_t HEIGHT = 6;
constexpr size_t WIDTH = 8;

unsigned int field[HEIGHT * WIDTH] = {
    1,1,1,1,1,1,1,1,
    1,0,0,1,1,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,1,1,0,0,1,
    1,0,0,1,1,0,0,1,
    1,1,1,1,1,1,1,1
};

template <size_t height, size_t width>
void flood_fill_scanline(int x, int y, auto& arr){
    if (arr[(y * width) + x]) return;

    struct RangeInfo {
        int line{};
        size_t begin{};
        size_t end{};
    };

    auto find_span = [](RangeInfo ri, auto& res, auto& arr){
        if (ri.line <= -1 || ri.line >= height) return; 
        size_t s = ri.begin;
        bool in_progress = !arr[(ri.line * width) + s];
        for (size_t i = ri.begin; i != ri.end + 1; ++i){
            if (arr[(ri.line * width) + i]){
                if (in_progress){
                    res.push({ri.line, s, i});
                    in_progress = false;
                }
            } else {
                if (!in_progress) {s = i; in_progress = true;}
            } 

            if (i + 1 == ri.end + 1 && in_progress){
                res.push({ri.line, s, ri.end});
            }
        }
      
    };

    auto fill_color = [](RangeInfo ri, auto& arr){
        for (int i = ri.begin; i != ri.end; ++i)
            arr[(ri.line * width) + i] = 2;
    };

    std::stack<RangeInfo> span_ranges{};

    int i1 = (y * width) + x - 1;
    int i2 = (y * width) + x + 1;

    while((!arr[i1] || !arr[i2])){
        if (!arr[i1])  {--i1;}
        if (!arr[i2])  {++i2;}
    }
    span_ranges.push({y, (i1 % width) + 1, (i2 % width)});

    while(!span_ranges.empty()){
        auto cur_val = span_ranges.top(); span_ranges.pop();
        fill_color(cur_val, arr);
        find_span({cur_val.line + 1, cur_val.begin, cur_val.end},  span_ranges, arr);
        find_span({cur_val.line - 1, cur_val.begin, cur_val.end},  span_ranges, arr);
    }
}

void print_field(){
    for (size_t i = 0; i < HEIGHT; ++i){
        for (size_t j = 0; j < WIDTH; ++j){
            std::cout << field[(i * WIDTH) + j] << ' ';
        }
        std::cout << '\n';
    }
}

int main(){
    print_field();
    std::cout << "\n\n";
    flood_fill_scanline<HEIGHT, WIDTH>(5,2, field);
    print_field();
}