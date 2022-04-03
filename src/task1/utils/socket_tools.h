#pragma once

struct addrinfo;

void create_dest_addrinfo(const char *address, const char *port, addrinfo *res_addr, int& err);

int create_socket(const char* port);


