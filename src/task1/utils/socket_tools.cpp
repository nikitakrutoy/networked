#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdio.h>

#include "socket_tools.h"

// Adaptation of linux man page: https://linux.die.net/man/3/getaddrinfo
static int get_dgram_socket(addrinfo *addr) {
    for (addrinfo *ptr = addr; ptr != nullptr; ptr = ptr->ai_next) {
        if (ptr->ai_family != AF_INET || ptr->ai_socktype != SOCK_DGRAM || ptr->ai_protocol != IPPROTO_UDP)
            continue;
        int sfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sfd == -1)
            continue;

        fcntl(sfd, F_SETFL, O_NONBLOCK);

        int trueVal = 1;
        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &trueVal, sizeof(int));

//        if (res_addr)
//            *res_addr = *ptr;
//        if (!should_bind)
//            return sfd;

        if (bind(sfd, ptr->ai_addr, ptr->ai_addrlen) == 0)
            return sfd;

        close(sfd);
    }
    return -1;
}

void create_dest_addrinfo(const char* address, const char* port, addrinfo* res_addr, int& err) {
    addrinfo hints;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    addrinfo *result = nullptr;
    err = getaddrinfo(address, port, &hints, &result);
    if (!err)
        *res_addr = *result;
}

int create_socket(const char* port) {
    addrinfo hints;
    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo *result = nullptr;
    if (getaddrinfo(nullptr, port, &hints, &result) != 0)
        return 1;

    int sfd = get_dgram_socket(result);

    //freeaddrinfo(result);
    return sfd;
}


int create_dgram_socket(const char *address, const char *port, addrinfo *res_addr) {
    addrinfo hints;
    memset(&hints, 0, sizeof(addrinfo));

//  bool isListener = !address;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
//  if (isListener)
    hints.ai_flags = AI_PASSIVE;

    addrinfo *result = nullptr;
    if (getaddrinfo(address, port, &hints, &result) != 0)
        return 1;

    int sfd = get_dgram_socket(result);

    //freeaddrinfo(result);
    return sfd;
}

