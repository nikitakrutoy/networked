
#ifndef CMAKE_PROJECT_TEMPLATE_UTILS_H
#define CMAKE_PROJECT_TEMPLATE_UTILS_H
#include <string>
#include <vector>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <sys/fcntl.h>


std::vector<std::string> split(std::string s, std::string delimiter);
long parse_uint(const char* s);


#endif //CMAKE_PROJECT_TEMPLATE_UTILS_H
