cmake_minimum_required(VERSION 3.10)

project(Anthill C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(anthill src/main.c)

target_include_directories(anthill PRIVATE include)

target_link_libraries(anthill PRIVATE Threads::Threads)

if(MSVC)
    target_compile_options(anthill PRIVATE /W4)
    target_compile_definitions(anthill PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(anthill PRIVATE -Wall -Wextra)
endif()

if(WIN32)
    target_link_libraries(anthill PRIVATE ws2_32)
endif()
