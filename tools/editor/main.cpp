#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <thread>
#include <string>
#include <fstream>
#include <atomic>
#include <shared_mutex>


class Buffer {
private:
    mutable std::shared_mutex mt;
protected:
    // start line
    size_t current = 0;
    std::vector<std::string> lines;
public:    
    Buffer(){
        lines.reserve(2);
    }
    void new_line() {
        std::unique_lock ul(mt);
        lines.push_back({});
        ++current;
    }
    void add(char sym) {
        std::unique_lock ul(mt);
        lines[current] += sym;
    }
    size_t get_current() const {
        std::shared_lock sl(mt);
        return current;
    }
    size_t size() const {
        std::shared_lock sl(mt);
        return lines.size();
    }
    void pop() {
        std::unique_lock ul(mt);
        if (!lines[current].empty()){
            lines[current].pop_back();
        } else {
            if (lines.size() > 1){
                lines.pop_back();
                --current;
            }
        }
    }

    template <typename Printer>
    void print(Printer& printer) {
        std::shared_lock sl(mt);
        printer(lines.cbegin(), lines.cend());
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
            ++current;
            lines.push_back(line + '\n');
        };
        if (lines.empty()) {
            lines.push_back({});
        } else {
            --current;
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
        char buffer[2];
        std::cin.read(buffer, 1);
        buffer[1] = '\0';
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

std::atomic_uint16_t del_count;
std::atomic_uint16_t update_count;
std::atomic_uint16_t new_line_count;
std::atomic<bool> buffer_save = false;

void command_handler(Buffer& buff) {
    std::thread([](Buffer& buff, std::atomic_uint16_t& del_count, std::atomic_uint16_t& update_count, std::atomic<bool>& buffer_save, std::atomic_uint16_t& new_line_count){
        int state; // 0 - empty, 1 - error, 2 - add data, 3 - del data
        for (;;) {
            auto val = input_checker(5, state);
            if (state == 2) {
                {
                    if (val == '\n') {
                        buff.add(val);
                        buff.new_line();
                        ++new_line_count;
                    } else if (val == 27) {
                        buffer_save.store(true);
                    } else {
                        buff.add(val);
                        ++update_count;
                    }
                }
            } else if (state == 3) {
                {
                    buff.pop();
                    ++del_count;
                }
            }
        }
    }, std::ref(buff),std::ref(del_count), std::ref(update_count), std::ref(buffer_save), std::ref(new_line_count)).detach();
}


struct Printer {
    template <typename Iter>
    void operator()(Iter it, Iter ite) {
        size_t counter = 1;
        for (; it != ite; ++it) {
            std::cout<< "|" << counter++ << "| " << *it;
        }
    }
};

int main(int argc, char *argv[]){
    if (argc != 2) {
        std::abort();
    }
    
    const char* filename = argv[1];
    FileBuffer b(filename);
    
    // run command handler thread
    set_buffered_input(false);
    command_handler(b);

    // clear screen before start
    std::cout << "\x1B[2J\x1B[H";

    Printer printer;

    for(;;) {
        // std::cout << "\x1B[2J\x1B[H";
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        {
            if (buffer_save.load() == true) {
                b.save_and_close();
                break;
            }
            if (new_line_count.load() > 0) {
                std::cout << "\x1B[2J\x1B[H";
                --new_line_count;
            }
            if (del_count.load() > 0) {
                std::cout << "\x1B[2J\x1B[H";
                --del_count;
            }
            std::cout << "\x1B[H";
            b.print(printer);
            std::cout << "\n\n------------------------\n\nCurrent:" << b.get_current() << "\nLines:" << b.size() << "\n";
            std::cout << std::flush;
        }
    }
}