#include <sys/socket.h>
#include <sys/select.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include "socket_tools.h"
#include <ctime>

#include <map>

struct user_info {
    std::string name;
    sockaddr addr;
    time_t time;
    bool is_alive;
};

std::map<std::string, user_info> users;

void process_data(char* buffer, int len, sockaddr from_addr, int sfd) {
    if (buffer[0] == 2) {
        auto user_info_iter = users.find(std::string(from_addr.sa_data));
        if (user_info_iter != users.end()) {
            user_info_iter->second.is_alive = true;
            user_info_iter->second.time = std::time(0);
        }
    }
    if (buffer[0] == 1) {
        users[std::string(from_addr.sa_data)] = user_info{
            std::string(buffer + 1),
            from_addr,
            std::time(0),
            true};
        std::string name = users[std::string(from_addr.sa_data)].name;
        for (auto pair : users) {
            std::string out_msg = name + " Connected";
            sockaddr to_addr = pair.second.addr;
            if (std::string(from_addr.sa_data) == std::string(to_addr.sa_data) || !pair.second.is_alive)
                continue;
            ssize_t res = sendto(sfd, out_msg.c_str(), out_msg.size(), 0, &to_addr, to_addr.sa_len);
            if (res == -1)
                std::cout << strerror(errno) << std::endl;
        }
    }
    if (buffer[0] == 0) {
        std::string in_msg = std::string(buffer + 1);
        std::string name = users[std::string(from_addr.sa_data)].name;
        for (auto pair : users) {
            std::string out_msg = name + ": " + in_msg;
            sockaddr to_addr = pair.second.addr;
            if (std::string(from_addr.sa_data) == std::string(to_addr.sa_data) || !pair.second.is_alive)
                continue;
            ssize_t res = sendto(sfd, out_msg.c_str(), out_msg.size(), 0, &to_addr, to_addr.sa_len);
            if (res == -1)
                std::cout << strerror(errno) << std::endl;
        }
    }
}

int main(int argc, const char **argv) {
    const char *port = "2023";

    int sfd = create_socket(port);

    if (sfd == -1)
        return 1;
    printf("listening!\n");
    sockaddr from_addr;
    memset(&from_addr, 0, sizeof(sockaddr));
    socklen_t from_addr_len = sizeof(from_addr);

    while (true) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sfd, &readSet);

        for (auto &pair2 : users) {
            if (std::time(0) - pair2.second.time > 10 && pair2.second.is_alive) {
                pair2.second.is_alive = false;
                std::string out_msg = pair2.second.name + " Disconnected";
                for (auto pair : users) {
                    sockaddr to_addr = pair.second.addr;
                    if (std::string(pair2.second.addr.sa_data) == std::string(to_addr.sa_data) || !pair.second.is_alive)
                        continue;
                    ssize_t res = sendto(sfd, out_msg.c_str(), out_msg.size(), 0, &to_addr, to_addr.sa_len);
                    if (res == -1)
                        std::cout << strerror(errno) << std::endl;
                }
            }
        }

        timeval timeout = {0, 100000}; // 100 ms
        select(sfd + 1, &readSet, NULL, NULL, &timeout);


        if (FD_ISSET(sfd, &readSet)) {
            constexpr size_t buf_size = 1000;
            static char buffer[buf_size];
            memset(buffer, 0, buf_size);
            ssize_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, &from_addr, &from_addr_len);
            if (numBytes > 0)
                process_data(buffer, numBytes, from_addr, sfd);// assume that buffer is a string
        }
    }
    return 0;
}
