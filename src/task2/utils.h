//
// Created by Nikita Krutoy on 04.04.2022.
//

#ifndef CMAKE_PROJECT_TEMPLATE_UTILS_H
#define CMAKE_PROJECT_TEMPLATE_UTILS_H

#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>

std::string get_uuid() {
    static std::random_device dev;
    static std::mt19937 rng(dev());

    std::uniform_int_distribution<int> dist(0, 15);

    const char *v = "0123456789abcdef";
    const bool dash[] = { 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0 };

    std::string res;
    for (int i = 0; i < 16; i++) {
        if (dash[i]) res += "-";
        res += v[dist(rng)];
        res += v[dist(rng)];
    }
    return res;
}


using time_point = std::chrono::system_clock::time_point;
std::string serialize_time_point( const time_point& time, const std::string& format)
{
    std::time_t tt = std::chrono::system_clock::to_time_t(time);
    std::tm tm = *std::gmtime(&tt); //GMT (UTC)
    std::stringstream ss;
    ss << std::put_time( &tm, format.c_str() );
    return ss.str();
}

#endif //CMAKE_PROJECT_TEMPLATE_UTILS_H
