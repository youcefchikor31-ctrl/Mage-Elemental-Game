cmake_minimum_required(VERSION 3.10)
project(MageElements CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(raylib REQUIRED)

add_executable(MageElements main.cpp)
target_link_libraries(MageElements PRIVATE raylib)
