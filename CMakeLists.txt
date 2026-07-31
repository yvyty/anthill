cmake_minimum_required(VERSION 3.10)
project(Anthill C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

if(CMAKE_C_COMPILE_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -03")
elseif(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /02")
endif()

include_directories(include)

add_executable(anthill src/main.c)

target_link_libraries(anthill PRIVATE Threads::Threads)
if(WIN32)
    target_link_libraries(anthill ws2_32)
endif()
