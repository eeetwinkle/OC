cmake_minimum_required(VERSION 3.20)

project(Lab3 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(WIN32)
    add_executable(lab3
        src/main.c
        src/process_windows.c
    )
else()
    add_executable(lab3
        src/main.c
        src/process_posix.c
    )
endif()

target_include_directories(lab3 PRIVATE include)