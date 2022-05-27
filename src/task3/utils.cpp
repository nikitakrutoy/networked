#include "utils.h"

#include <string>
#include <vector>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <sys/fcntl.h>


std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos = 0;
    std::string token;
    std::vector<std::string> result;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        s.erase(0, pos + delimiter.length());
        result.push_back(token);
    }
    result.push_back(s);
    return result;
}


long parse_uint(const char* s) {
    errno = 0;
    char* p_end;
    const long n = std::strtol(s, &p_end, 10);
    if (errno != 0) return -1;
    else return n;
}


