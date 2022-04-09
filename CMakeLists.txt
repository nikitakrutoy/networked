cmake_minimum_required(VERSION 3.2)

project(cmake-project-template)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

set(GIT_SUBMODULES_RECURSE 1)

add_subdirectory(lib/bgfx.cmake)

include(CPM.cmake)
CPMAddPackage("gh:lsalzman/enet@1.3.17")
CPMAddPackage("gh:glfw/glfw#3.3.6")

#add_subdirectory(src/task1)
add_subdirectory(src/task3)
# add_subdirectory(test)


