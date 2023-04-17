cmake_minimum_required(VERSION 3.22)
project(os2new)

set(CMAKE_CXX_STANDARD 14)

add_executable(os2new demo_singInt_handler.c
        hi.cpp)
