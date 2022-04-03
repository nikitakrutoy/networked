cmake_minimum_required(VERSION 3.2)
project(utils C CXX)

set(SOURCE_FILES
    socket_tools.h
    socket_tools.cpp
)

add_library(utils SHARED STATIC ${SOURCE_FILES})

install(TARGETS utils DESTINATION ${DIVISIBLE_INSTALL_LIB_DIR})
install(FILES socket_tools.h DESTINATION ${DIVISIBLE_INSTALL_INCLUDE_DIR})
