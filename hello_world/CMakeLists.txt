cmake_minimum_required(VERSION 3.20)

project(HelloWorld LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(hello src/main.c)

set(CMAKE_BUILD_TYPE Debug)