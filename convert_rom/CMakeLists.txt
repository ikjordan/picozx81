set(PROJECT convert_rom)
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)

project(${PROJECT} C)

add_executable(${PROJECT} convert_rom.c)