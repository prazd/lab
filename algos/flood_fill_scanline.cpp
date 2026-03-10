#include <iostream>
#include <stack>

constexpr size_t HEIGHT = 6;
constexpr size_t WIDTH = 8;

unsigned int field[HEIGHT * WIDTH] = {
    1,1,1,1,1,1,1,1,
    1,1,1,0,0,1,0,1,
    1,1,0,0,1,0,0,1,
    1,0,0,1,1,0,0,1,
    1,0,0,1,1,0,0,1,
    1,1,1,1,1,1,1,1
};

template <int height, int width>
void flood_fill_scanline(int x, int y, auto& arr){
    if (arr[(y * width) + x]) return;

    struct RangeInfo {
        int line{};
        int begin{};
        int end{};
    };

    auto find_span = [](RangeInfo ri, auto& res, auto& arr){
        if (ri.line <= -1 || ri.line >= height) return;
        int point = ri.begin;
        bool start_span = !arr[(ri.line * width) + point];
        while(true){
            if (start_span) {
                int i1 = (ri.line * width) + point;
                int i2 = i1;
                while((((i1 % width) != 0 && !arr[i1]) || ((i2 % width) != width - 1 && !arr[i2]))){
                    i1 -= (!arr[i1]);
                    i2 += (!arr[i2]);
                }
                res.push({ri.line, (i1 % width) + 1, (i2 % width)});
                start_span = false;
                point = (i2 % width);
            } else {
                if ((point % width) == width - 1 ||
                        (point % width >= ri.end) && arr[(ri.line * width) + (point % width)]) {
                            break;
                        }
                else {
                    ++point;
                    if (!(arr[(ri.line * width) + point]))
                        start_span = true;
                }
            }
        }
    };

    auto fill_color = [](RangeInfo ri, auto& arr){
        for (int i = ri.begin; i != ri.end; ++i)
            arr[(ri.line * width) + i] = 8;
    };
    std::stack<RangeInfo> span_ranges{};
    find_span({y, x, x},  span_ranges, arr);
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
    flood_fill_scanline<HEIGHT, WIDTH>(3,1, field);
    print_field();
}