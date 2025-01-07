#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <thread>
#include <string>
#include <fstream>
#include <atomic>
#include <shared_mutex>
#include <csignal>
#include <tuple>

constexpr char CTRL_W = 0x17;
constexpr char CTRL_U = 0x15;
constexpr char CTRL_F = 0x06;
constexpr char DELETE = 127;
constexpr char ESC = '\x1B';
constexpr char SPACE = ' ';

constexpr const char* SHOW_CURSOR_CMD = "\x1B[?25h";
constexpr const char* HIDE_CURSOR_CMD = "\x1B[?25l";
constexpr const char* FULL_CLEAN_SCREEN_CMD = "\x1B[2J\x1B[H";
constexpr const char* RED_COLOR_CMD = "\x1B[31m";
constexpr const char* DEFAULT_COLOR_CMD = "\x1B[0m";
constexpr const char* SET_CURSOR_TO_BEGIN_CMD = "\x1B[H";
constexpr const char* CLEAN_CURRENT_LINE_CMD = "\x1B[2K";
constexpr const char* CLEAN_BELOW_CMD = "\x1B[0J";

std::atomic<bool> buffer_save = false;
std::atomic<bool> update_screen = false;
std::atomic<bool> terminate_flag = false;
std::condition_variable render_cv;
std::mutex render_mutex;

class Buffer {
protected:
    std::vector<std::string> lines;
public:
    void new_line(size_t row) {
        if (row == lines.size()-1) {
            lines.push_back({});
        } else {
            lines.insert(lines.begin()+(row+1), "");
        }
    }

    void add(size_t index, char sym) {
        lines[index] += sym;
    }

    void insert(size_t row, size_t col, char sym) {
        lines[row].insert(lines[row].begin() + col, sym);
    }

    size_t size() const {
        return lines.size();
    }

    void pop_line(size_t row) {
        if (row == lines.size()-1)
            lines.pop_back();
        else {
            lines.erase(lines.begin()+row);
        }
    }

    void del_sym(size_t row, size_t col) {
        lines[row].erase(lines[row].begin() + col);
    }

    size_t pop_to_space(size_t row, size_t col) {
        auto& cur = lines[row];
        auto begin = cur.find_last_of(SPACE, col);
        auto end = col;
        if (begin != std::string::npos) {
            cur.erase(cur.begin() + begin, cur.begin() + end);
            return end - begin;
        }
        cur.erase(0, end);
        return col;
    }

    void pop_to_begin(size_t row, size_t col) {
        lines[row].erase(0, col);
    }

    void pop_sym_by_index(size_t index) {
        lines[index].pop_back();
    }

    bool empty_line(size_t index) {
        return lines[index].empty();
    }

    size_t size_by_index(size_t index) {
        return lines[index].size();
    }

    auto cbegin() const {
        return lines.cbegin();
    }

    auto cend() const {
        return lines.cend();
    }
};

class FileBuffer: public Buffer {
    const char* filename;
public:    
    FileBuffer(const char* filename): filename(filename){
        std::ifstream file(filename);
        if (!file) {
            std::abort();
        }
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        };
        if (lines.empty()) {
            lines.push_back({});
        }
        file.close();
    }
    void save_and_close() {
        std::ofstream file(filename);
        for (auto& s: lines)
            file << s << "\n";
        file.close();
    }
};

template <typename B = FileBuffer>
class BufferManager {
    mutable std::shared_mutex mt;
    B b;
    size_t row;
    size_t col;
public:
    BufferManager(const char* filename): b(filename), row(b.size()-1), col(b.size_by_index(row)){}
    void pop_line() {
        if (b.size() > 1){
            b.pop_line(row);
            if (row) --row;
            col = b.size_by_index(row);
        }
    }
    void pop() {
        std::unique_lock ul(mt);
        if (!b.empty_line(row)) {
            if (col == b.size_by_index(row)) {
                b.pop_sym_by_index(row);
                --col;
            } else if (col > 0) {
                b.del_sym(row, col-1);
                --col;
            }
        } else {
            pop_line();
        }
    }

    void pop_word() {
        std::unique_lock ul(mt);
        if (!b.empty_line(row)) {
            col -= b.pop_to_space(row, col);
        } else {
            pop_line();
        }
    }

    void pop_to_begin() {
        std::unique_lock ul(mt);
        if (col > 0) {
            b.pop_to_begin(row, col);
            col = 0;
        } else {
            pop_line();
        }
    }

    void new_line() {
        std::unique_lock ul(mt);
        b.new_line(row);
        ++row;
        col = 0;
    }

    void add(char sym) {
        std::unique_lock ul(mt);
        if (col == b.size_by_index(row)) {
            b.add(row, sym);
        } else {
            b.insert(row, col, sym);
        }
        ++col;
    }

    size_t get_row() const {
        std::shared_lock sl(mt);
        return row;
    }

    size_t get_col() const {
        std::shared_lock sl(mt);
        return col;
    }

    void save_and_close() {
        std::unique_lock ul(mt);
        b.save_and_close();
    }

    template <typename Printer>
    void print(Printer& p) {
        std::shared_lock sl(mt);
        auto copy = b;
        sl.unlock();
        p.print(copy.cbegin(), copy.cend(), row, col);
    }

    void left() {
        if (col > 0) {
            {
                std::unique_lock ul(mt);
                --col;
            }
        }
    }

    void right() {
        if (col < b.size_by_index(row)) {
            {
                std::unique_lock ul(mt);
                ++col;
            }
        }
    }

    void up() {
        if (row > 0) {
            {
               std::unique_lock ul(mt);
                --row;
                if (col >= b.size_by_index(row)) {
                    col = b.size_by_index(row); 
                }
            }
        }
    }
    
    void down() {
        if (row < b.size()-1) {
            {
                std::unique_lock ul(mt);
                ++row;
                if (col >= b.size_by_index(row)) {
                    col = b.size_by_index(row); 
                }
            }
        }
    }

};

class RenderManager {
public:    
    RenderManager() {
        std::cout << FULL_CLEAN_SCREEN_CMD;
        std::cout << HIDE_CURSOR_CMD;
    }
    struct LinePrinter {
        template <typename Iter>
        void operator()(Iter it, Iter ite, size_t row, size_t col) {
            size_t counter{};
            for (; it != ite; ++it, ++counter) {
                std::cout << CLEAN_CURRENT_LINE_CMD;
                if (counter == row) {
                    print_cursor(*it, col);
                } else {
                   std::cout<< "|*| " << *it;
                }
                std::cout << "\n";
            }
        }
        void print_cursor(const std::string& str, size_t pos) {
            std::cout << CLEAN_CURRENT_LINE_CMD;
            auto sub1 = str.substr(0, pos);
            auto sub2 = str.substr(pos);
            std::cout << str.size();
            std::cout<< "|*| ";
            std::cout << sub1;
            std::cout << RED_COLOR_CMD;
            std::cout << "|"; 
            std::cout << DEFAULT_COLOR_CMD;
            std::cout << sub2;
        }
    } printer;

    template <typename Iter>
    void print(Iter it, Iter ite, size_t row, size_t col) {
        std::cout << SET_CURSOR_TO_BEGIN_CMD;
        printer(it, ite, row, col);
        std::cout << CLEAN_BELOW_CMD;
        std::cout << std::flush;
    }

    template <typename... Info>
    void print_info(Info&&... info) {
       (std::cout << ... << info) << "\n";
       std::cout << std::flush;
    }
    ~RenderManager() {
        std::cout << SHOW_CURSOR_CMD;
    }
};


template <typename BM = BufferManager<FileBuffer>>
struct CommandHandler {
    enum class Command: char {
        Empty, Error, Add, Del, Up, Down, Right, Left, SaveExit,
        // ShortCuts
        PopWord,   // Ctrl + W
        PopToBegin // Ctrl + U
    };

    struct RenderNotifier {
        RenderNotifier() = default;
        ~RenderNotifier(){
            update_screen.store(true);
            render_cv.notify_one();
        }
    };

    CommandHandler() {
        set_buffered_input(false);
    }

    std::tuple<int, Command> input(int timeout_ms) {
        struct pollfd fds[1];
        fds[0].fd = 0;
        fds[0].events = POLLIN;

        int ret = poll(fds, 1, timeout_ms);
        if (ret == -1) {
            return {ret, Command::Error};
        } else if (ret == 0) {
            return {ret, Command::Empty};
        }
        if (fds[0].revents & POLLIN) {
            char buffer[4];
            std::cin.read(buffer, 1);
            auto fsym = buffer[0];
            switch (fsym) {
                case ESC: {
                    std::cin.read(buffer + 1, 2);  // Read 2 syms
                    if (buffer[1] == '[') {
                        switch (buffer[2]) {
                            case 'A': {return {buffer[2], Command::Up     };}
                            case 'B': {return {buffer[2], Command::Down   };}
                            case 'C': {return {buffer[2], Command::Right  };}
                            case 'D': {return {buffer[2], Command::Left   };}
                            default:  {return {buffer[2], Command::Empty  };}
                        }
                    }
                    break;
                }
                case CTRL_W:
                    return  {buffer[1], Command::PopWord};
                case CTRL_U:
                    return  {buffer[1], Command::PopToBegin};
                case CTRL_F:
                    return  {buffer[1], Command::SaveExit};
                case DELETE:
                    return  {fsym,      Command::Del};
                default:
                    return {fsym, Command::Add};
            }
        }
        return {ret, Command::Error};
   }

   void set_buffered_input(bool enable) {
        static struct termios oldt, newt;
        if (!enable) {
            tcgetattr(STDIN_FILENO, &oldt); 
            newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        } else {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        }
    }

    void operator()(BM& bm) {
        for (;;) {
            auto [sym, cmd] = input(5);
            switch (cmd) {
                case Command::Empty: continue;
                case Command::Add:        { RenderNotifier rn; sym == '\n' ? (bm.new_line()) : (bm.add(sym)); break;}

                case Command::Del:        { RenderNotifier rn; bm.pop();          break;}
                case Command::PopWord:    { RenderNotifier rn; bm.pop_word();     break;}
                case Command::PopToBegin: { RenderNotifier rn; bm.pop_to_begin(); break;}
                case Command::Up:         { RenderNotifier rn; bm.up();           break;}
                case Command::Down:       { RenderNotifier rn; bm.down();         break;}
                case Command::Right:      { RenderNotifier rn; bm.right();        break;}
                case Command::Left:       { RenderNotifier rn; bm.left();         break;}
                
                case Command::SaveExit:   { buffer_save.store(true); render_cv.notify_one(); return;}
                case Command::Error:      { break;} // 
            }
        }
    }
};

void handle_signal(int signal) {
    std::cout << SHOW_CURSOR_CMD;
    terminate_flag.store(true);
    render_cv.notify_one();
    exit(0);
}

int main(int argc, char *argv[]){
    if (argc != 2) {
        std::abort();
    }

    std::signal(SIGINT, handle_signal);
    
    const char* filename = argv[1];
    BufferManager<FileBuffer> b(filename);
    
    
    std::thread(CommandHandler{}, std::ref(b)).detach();
    
    RenderManager rm;
    b.print(rm);

    for(;;) {
        {
            std::unique_lock ul(render_mutex);
            render_cv.wait(ul, [&]{return update_screen.load() || buffer_save.load() || terminate_flag.load();});
            if (buffer_save.load()) {
                b.save_and_close();
                break;
            } else if (terminate_flag.load()) {
                break;
            }
            b.print(rm);
            rm.print_info("\nLines:", b.get_row(), "\nCol:", b.get_col());
            update_screen.store(false);
        }
    }
}