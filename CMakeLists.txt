cmake_minimum_required(VERSION 3.16)

project(sig_gen C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_executable(sig_gen
    main.c
    renderer.c
    signal.c
    font_embedded.c
)

target_compile_options(sig_gen PRIVATE -Wall -Wextra)

target_link_libraries(sig_gen
    SDL2::SDL2
    SDL2_ttf::SDL2_ttf
    m
)
