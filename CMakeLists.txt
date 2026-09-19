cmake_minimum_required(VERSION 3.10)

project(Anthill C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

function(anthill_apply_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()

add_library(anthill_core STATIC src/args.c src/scan.c)

target_include_directories(anthill_core PUBLIC include)

anthill_apply_warnings(anthill_core)

add_executable(anthill src/main.c)

target_link_libraries(anthill PRIVATE anthill_core Threads::Threads)

anthill_apply_warnings(anthill)

if(WIN32)
    target_link_libraries(anthill PRIVATE ws2_32)
endif()

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()