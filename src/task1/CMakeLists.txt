cmake_minimum_required(VERSION 3.2)
project(netwroked)

add_subdirectory(utils)
set(CLIENT_SOURCE_FILES client/client.cpp)
set(SERVER_SOURCE_FILES server/server.cpp)

add_executable(task1-client ${CLIENT_SOURCE_FILES})
add_executable(task1-server ${SERVER_SOURCE_FILES})
target_link_libraries(task1-client utils)
target_link_libraries(task1-server utils)
#install(TARGETS client DESTINATION ${DIVISIBLE_INSTALL_BIN_DIR})
#install(TARGETS server DESTINATION ${DIVISIBLE_INSTALL_BIN_DIR})
