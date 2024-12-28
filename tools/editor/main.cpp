#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <thread>
#include <string>
#include <fstream>
#include <atomic>

class Buffer {
protected:    
    std::string syms;
public:    
    Buffer(){
        syms.reserve(100);
    }
    void add(char sym) {
        syms+=sym;
    }
    void pop() {
        if (!syms.empty())
            syms.pop_back();
    }
    const std::string& get() const {
        return syms;
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
            syms+=line + '\n';
        };
        file.close();
    }
    void save_and_close() {
        std::ofstream file(filename);
        file << syms;
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
std::atomic<bool> buffer_save = false;

void command_handler(Buffer& buff) {
    std::thread([](Buffer& buff, std::atomic_uint16_t& del_count, std::atomic_uint16_t& update_count, std::atomic<bool>& buffer_save){
        int state; // 0 - empty, 1 - error, 2 - add data, 3 - del data
        for (;;) {
            auto val = input_checker(5, state);
            if (state == 2) {
                {
                    if (val == 27) {
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
    }, std::ref(buff),std::ref(del_count), std::ref(update_count), std::ref(buffer_save)).detach();
}

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
    auto& data = b.get();

    for(;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        {
            if (buffer_save.load() == true) {
                b.save_and_close();
                break;
            }
            if (del_count.load() > 0) {
                std::cout << "\x1B[2J\x1B[H";
                del_count--;
            }
            std::cout << "\x1B[H";
            std::cout << data;
            std::cout << std::flush;
        }
    }
}