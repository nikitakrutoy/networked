#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include "socket_tools.h"
#include <vector>

int main(int argc, const char **argv) {
//    const char *port = "2022";
    const char *server_port = "2023";
    std::string name;
    std::string port;

    if (argc > 2) {
        name = std::string(argv[1]);
        port = std::string(argv[2]);
    }
    else
        return 2;

    addrinfo resAddrInfo;
    int err = 0;
    create_dest_addrinfo("localhost", server_port, &resAddrInfo, err);
    if (err) {
        std::cout << "Could not create dest addrinfo";
    }
    int sfd = create_socket(port.c_str());

    if (sfd == -1) {
        printf("Cannot create a socket\n");
        return 1;
    }

    // Initialize connection
    std::vector<char> buffer(name.size() + 1);
    buffer[0] = 1;
    memcpy(buffer.data() + 1, name.c_str(), name.size());

    ssize_t res = sendto(sfd, buffer.data(), buffer.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
    if (res == -1) {
        std::cout << strerror(errno) << std::endl;
        std::cout << "Could not init connection with server" << std::endl;
    }

    while (true) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sfd, &readSet);
        FD_SET(0, &readSet);

        timeval timeout = {0, 100000}; // 100 ms
        select(sfd + 1, &readSet, NULL, NULL, &timeout);

        // keep alive
        buffer.resize(1);
        buffer[0] = 2;
        ssize_t res = sendto(sfd, buffer.data(), buffer.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
        if (res == -1) {
            std::cout << strerror(errno) << std::endl;
        }


        if (FD_ISSET(0, &readSet)) {
            std::string input;
//            printf(">");
            std::getline(std::cin, input);
            buffer.resize(input.size() + 1);
            buffer[0] = 0;
            memcpy(buffer.data() + 1, input.c_str(), input.size());
            ssize_t res = sendto(sfd, buffer.data(), buffer.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);
            if (res == -1) {
                std::cout << strerror(errno) << std::endl;
            }
        }

        if (FD_ISSET(sfd, &readSet)) {
            constexpr size_t buf_size = 1000;
            static char data[buf_size];
            memset(data, 0, buf_size);
            ssize_t numBytes = recvfrom(sfd, data, buf_size - 1, 0, nullptr, nullptr);

            if (numBytes > 0)
                printf("%s\n", data); // assume that buffer is a string
        }
    }
    return 0;
}
