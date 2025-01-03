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

class Buffer {
protected:
    std::vector<std::string> lines;
public:    
    Buffer(){
        lines.reserve(2);
    }

    void new_line() {
        lines[lines.size()-1]+='\n';
        lines.push_back({});
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

    void pop_line() {
        lines.pop_back();
    }
    void del_sym(size_t row, size_t col) {
        lines[row].erase(lines[row].begin() + col);
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
            lines.push_back(line + '\n');
        };
        if (lines.empty()) {
            lines.push_back({});
        }
        file.close();
    }
    void save_and_close() {
        std::ofstream file(filename);
        for (auto& s: lines)
            file << s;
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
            if (b.size() > 1){
                b.pop_line();
                --row;
                col = b.size_by_index(row);
            }
        }
    }

    void new_line() {
        std::unique_lock ul(mt);
        b.new_line();
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

    std::pair<size_t, size_t> get_current() const {
        std::shared_lock sl(mt);
        return {row, col};
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
                    col = b.size_by_index(row)-1; 
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
                    col = b.size_by_index(row)-1; 
                }
            }
        }
    }

};

class RenderManager {
public:    
    RenderManager() {
        std::cout << "\x1B[2J\x1B[H";
        std::cout << "\x1B[?25l"; // hide apparat cursor
    }
    struct LinePrinter {
        template <typename Iter>
        void operator()(Iter it, Iter ite, size_t row, size_t col) {
            size_t counter{};
            for (; it != ite; ++it, ++counter) {
                std::cout << "\x1B[2K";
                if (counter == row) {
                    print_cursor(*it, col);
                } else {
                   std::cout<< "|*| " << *it;
                }
            }
        }
        void print_cursor(const std::string& str, size_t pos) {
            std::cout << "\x1B[2K";
            auto sub1 = str.substr(0, pos);
            auto sub2 = str.substr(pos);
            std::cout<< "|*| ";
            std::cout << sub1;
            std::cout << "\x1B[31m"; // set red color
            std::cout << "|";
            std::cout << "\x1B[0m";  // set default
            std::cout << sub2;
        }
    } printer;

    void clear_current() {
        std::cout << "\x1B[2K";
    }

    template <typename Iter>
    void print(Iter it, Iter ite, size_t row, size_t col) {
        std::cout << "\x1B[H";
        printer(it, ite, row, col);
        std::cout << "\x1B[0J";
        std::cout << std::flush;
    }

    template <typename... Info>
    void print_info(Info&&... info) {
       (std::cout << ... << info) << "\n";
       std::cout << std::flush;
    }
    ~RenderManager() {
        std::cout << "\x1B[?25h"; // show apparat cursor;
    }
};

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

char input_checker(int timeout_ms, int& state) {
    struct pollfd fds[1];
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    int ret = poll(fds, 1, timeout_ms);
    if (ret == -1) {
        state = 1; // error
        return ret;
    } else if (ret == 0) {
        state = 0; // empty
        return ret;
    }
    if (fds[0].revents & POLLIN) {
        char buffer[4];
        std::cin.read(buffer, 1);
        if (buffer[0] == '\x1B') {         // Если это ESC
            std::cin.read(buffer + 1, 2);  // Читаем ещё два символа
            if (buffer[1] == '[') {
                switch (buffer[2]) {
                        case 'A': {state = 4; return buffer[2];} // Стрелка вверх
                        case 'B': {state = 5; return buffer[2];} // Стрелка вниз
                        case 'C': {state = 6; return buffer[2];} // Стрелка вправо
                        case 'D': {state = 7; return buffer[2];} // Стрелка влево
                    }
            } else {
                state = 8;
                return buffer[0];
            }
        }
        if (buffer[0] == 127) {
            state = 3; // del
        } else {
            state = 2; // add
        }
        return buffer[0];
    }
    state = 2;
    return ret;
}

std::atomic<bool> buffer_save = false;
std::atomic<bool> update_screen = false;
std::atomic<bool> terminate_flag = false;
std::condition_variable render_cv;
std::mutex render_mutex;

template <typename BM = BufferManager<FileBuffer>>
void command_handler(BM& bm) {
    std::thread([](BM& bm, std::atomic<bool>& buffer_save, std::atomic<bool>& update_screen, std::condition_variable& render_cv){
        int state; // 0 - empty, 1 - error, 2 - add data, 3 - del data, 4 - up, 5 - down, 6 - right, 7 - left, 8 - save and exit
        for (;;) {
            auto val = input_checker(5, state);
            switch (state) {
                case 0: continue;
                case 2: {
                    if (val == '\n') {
                        bm.new_line();
                        update_screen.store(true);
                    } else {
                        bm.add(val);
                        update_screen.store(true);
                    }
                    render_cv.notify_one();
                    break;
                }
                case 3:{
                    bm.pop();
                    update_screen.store(true);
                    render_cv.notify_one();
                    break;
                }
                case 4: {bm.up();    update_screen.store(true); render_cv.notify_one(); break;}
                case 5: {bm.down();  update_screen.store(true); render_cv.notify_one(); break;}
                case 6: {bm.right(); update_screen.store(true); render_cv.notify_one(); break;}
                case 7: {bm.left();  update_screen.store(true); render_cv.notify_one(); break;}
                case 8: {buffer_save.store(true); render_cv.notify_one(); return;}
            }
        }
    }, std::ref(bm), std::ref(buffer_save), std::ref(update_screen), std::ref(render_cv)).detach();
}

void handle_signal(int signal) {
    std::cout << "\x1B[?25h"; // show apparat cursor;
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
    
    // run command handler thread
    set_buffered_input(false);
    command_handler(b);

    RenderManager rm;
    
    
    b.print(rm); // print full
    for(;;) {
        {
            std::unique_lock ul(render_mutex);
            render_cv.wait(ul, [&]{return update_screen.load() || buffer_save.load() || terminate_flag.load();});
            if (buffer_save.load() == true) {
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