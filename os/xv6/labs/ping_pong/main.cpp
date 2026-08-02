#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <cstring>

void process(const char* msg, char data, int(&p_r)[2], int(&p_w)[2]){
    size_t count = 0;
    char count_buffer[21]{};
    count_buffer[20] = ':';
    std::array<char, 5 + std::size(count_buffer)> cout_d{};
    std::copy(msg, msg + 6, std::end(cout_d)-5);
    for (size_t i = 0; i < 1000; ++i){
        read(p_r[0], &data, 1);
        std::to_chars(std::begin(count_buffer), std::begin(count_buffer) + 20, count++);
        std::copy(std::begin(count_buffer), std::end(count_buffer), std::begin(cout_d));
        write(1, cout_d.data(), cout_d.size()); // ping\n / pong\n
        write(p_w[1], &data, 1);
    }
}

int main(int argc, char* argv[]){
    int ping_fd[2]{};
    int pong_fd[2]{};
      
    if (pipe(ping_fd) == -1) return -1;
    if (pipe(pong_fd) == -1) return -1;
 
    char data = 42;
    pid_t pong_id{};
    pong_id = fork();
    if (pong_id) {
        close(ping_fd[1]);
        close(pong_fd[0]);

        write(pong_fd[1], &data, 1);
        process("ping\n", data, ping_fd, pong_fd);
        int status{};
        wait(&status);

        close(ping_fd[0]);
        close(pong_fd[1]);
    } else {
        close(ping_fd[0]);
        close(pong_fd[1]);
        process("pong\n", data, pong_fd, ping_fd);
        close(ping_fd[1]);
        close(pong_fd[0]);
    }
}
