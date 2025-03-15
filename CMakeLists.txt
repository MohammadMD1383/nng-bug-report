cmake_minimum_required(VERSION 3.31)
project(test)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(nng-stable)
link_libraries(nng)

add_executable(test main.cpp nng++.cc nng++.h)
