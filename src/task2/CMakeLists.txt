cmake_minimum_required(VERSION 3.2)
project(netwroked)

#find_package(ENet REQUIRED)

set(CLIENT_SOURCE_FILES client.cpp)
set(LOBBY_SOURCE_FILES lobby.cpp)
set(SERVER_SOURCE_FILES server.cpp)

#include_directories(${ENET_INCLUDE_DIRS})

add_executable(task2-client ${CLIENT_SOURCE_FILES})
add_executable(task2-lobby ${LOBBY_SOURCE_FILES})
add_executable(task2-server ${SERVER_SOURCE_FILES})

target_link_libraries(task2-client enet)
target_link_libraries(task2-lobby enet)
target_link_libraries(task2-server enet)

if (enet_ADDED)
    target_include_directories(task2-client
            PUBLIC
            $<BUILD_INTERFACE:${enet_SOURCE_DIR}/include>
            )
    target_include_directories(task2-lobby
            PUBLIC
            $<BUILD_INTERFACE:${enet_SOURCE_DIR}/include>
            )
    target_include_directories(task2-server
            PUBLIC
            $<BUILD_INTERFACE:${enet_SOURCE_DIR}/include>
            )
endif()