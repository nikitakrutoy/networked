cmake_minimum_required(VERSION 3.2)
project(netwroked)
set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED)

set(APP_SOURCE_FILES app.cpp main.cpp protocol.cpp)
set(SERVER_SOURCE_FILES server.cpp protocol.cpp)

add_executable(task3-app ${APP_SOURCE_FILES})
add_executable(task3-server ${SERVER_SOURCE_FILES})

target_link_libraries(task3-app glfw)
target_link_libraries(task3-app OpenGL::GL)

target_link_libraries(task3-app bgfx meshoptimizer bx)

target_link_libraries(task3-app enet)
target_link_libraries(task3-server enet)

if (enet_ADDED)
    target_include_directories(task3-app PUBLIC
            $<BUILD_INTERFACE:${enet_SOURCE_DIR}/include>)
    target_include_directories(task3-server PUBLIC
            $<BUILD_INTERFACE:${enet_SOURCE_DIR}/include>)
endif()

if (glfw_ADDED)
    target_include_directories(task3-app PUBLIC
            $<BUILD_INTERFACE:${glfw_SOURCE_DIR}/include>)
endif()

target_include_directories(task3-app PUBLIC
        ${BGFX_DIR}/examples/common>)

target_include_directories(task3-app PUBLIC
        ${BX_DIR}/include>)
target_include_directories(task3-app PUBLIC
        ${BGFX_DIR}/include>)

#add_subdirectory(debugdraw)

target_link_libraries(task3-app example-common)
target_include_directories(task3-app PUBLIC
        ${BGFX_DIR}/examples/common)