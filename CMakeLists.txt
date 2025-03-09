cmake_minimum_required(VERSION 3.26)
project(OSy)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(sem3/Lab1)
add_subdirectory(sem3/Lab2)
add_subdirectory(sem3/Lab3)
add_subdirectory(sem3/Lab4)
add_subdirectory(sem4)
add_executable(test test.c)