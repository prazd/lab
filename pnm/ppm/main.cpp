#include <iostream>
#include <fstream>
#include <sstream>

constexpr size_t TILE_X = 10;
constexpr size_t TILE_Y = 10;
constexpr size_t HEIGHT = TILE_Y * 40;
constexpr size_t WIDTH = TILE_X * 40;
constexpr size_t GENERATIONS = 200;
constexpr unsigned char BLACK = 0;
constexpr unsigned char WHITE = 255;
const auto BLACK_ADDR = reinterpret_cast<const char*>(&BLACK);
const auto WHITE_ADDR = reinterpret_cast<const char*>(&WHITE);

template <size_t height, size_t width>
struct GoL {
  class Cell{
    //       neighbours        
    // 0 0 0 [0 0 0 0] [0]
    //                active
    unsigned char state{};
public:
    void set_alive_neighbours(unsigned char val) {
      state &= 0x1;
      state |= ((val & 0xf) << 1);
    }

    void set_active(bool val) {
        state &= 0xfe;
        state |= val;
    }

    unsigned char get_alive_neighbours() const {
      return (state >> 1) & 0xf;
    }

    bool active() const {
      return state & 1;
    }
  };

  Cell board[height][width]{};
 
  void step() {
    next_gen_neighbours();
    update_active();
  }

  void next_gen_neighbours() {
    for (int i = 0; i < height; ++i) {
      for (int j = 0; j < width; ++j) {
        unsigned char count = 0;
        count += board[i][(width + (j - 1))%width].active();
        count += board[i][(j + 1)%width].active();
        count += board[(i + 1)%height][j].active();
        count += board[(height +(i - 1))%height][j].active();

        count += board[(i + 1)%height][(j + 1)%width].active();
        count += board[(i + 1)%height][(width + (j - 1))%width].active();
        count += board[(height + (i - 1))%height][(j + 1)%width].active();
        count += board[(height + (i - 1))%height][(width + (j - 1))%width].active();
        board[i][j].set_alive_neighbours(count);
      }
    }
  }

  void update_active() {
      for (int i = 0; i < height; ++i) {
          for (int j = 0; j < width; ++j) {
              auto& cell = board[i][j];
              auto alive_neighbours = cell.get_alive_neighbours();
              if (cell.active() && (alive_neighbours < 2 || alive_neighbours > 3))
                cell.set_active(false);
              else if (!cell.active() && alive_neighbours == 3)
                cell.set_active(true);
          }
     }
}
};

void seed_glider(auto& g) {
  g.board[5][7].set_active(true);
  g.board[6][8].set_active(true);
  g.board[7][8].set_active(true);
  g.board[7][7].set_active(true);
  g.board[7][6].set_active(true);
}


int main(){
    std::stringstream ss;
    std::stringstream filename;
    GoL<HEIGHT/TILE_Y, WIDTH/TILE_X> g;
    seed_glider(g);
    
    for (int f = 0; f < GENERATIONS; ++f){
        ss << "P6\n";
        ss << HEIGHT << ' ' << WIDTH << '\n';
        ss << 255 << '\n';
       for (int i = 0; i < HEIGHT; ++i){
         for (int j = 0; j < WIDTH; ++j){
              if (i % TILE_Y == 0 || j % TILE_X == 0) {
                    // lines
                    ss.write(BLACK_ADDR,1);
                    ss.write(BLACK_ADDR,1);
                    ss.write(BLACK_ADDR,1); 
               } else {
                  if (g.board[i/TILE_Y][j/TILE_X].active()) {
                      ss.write(BLACK_ADDR,1);
                      ss.write(BLACK_ADDR,1);
                      ss.write(BLACK_ADDR,1); 
                  } else {
                      ss.write(WHITE_ADDR,1);
                      ss.write(WHITE_ADDR,1);
                      ss.write(WHITE_ADDR,1); 
                  }
            }
        }
    }
      filename << "res-" << f << ".ppm";
      auto s = std::ofstream(filename.str());
      s << ss.str();
      s.close();
      filename.str("");
      ss.str("");
      g.step();
    }
}
