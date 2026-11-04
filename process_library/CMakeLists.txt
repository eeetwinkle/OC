cmake_minimum_required(VERSION 3.20)

project(ProcessLibrary LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Выбираем реализацию библиотеки для текущей ОС.
if(WIN32)
    add_library(process STATIC src/process_windows.c)
else()
    add_library(process STATIC src/process.c)
endif()

target_include_directories(process PUBLIC include)

# Создаём тестовую программу.
add_executable(test_process tests/test_process.c)

target_link_libraries(test_process PRIVATE process)