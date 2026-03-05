cmake_minimum_required(VERSION 3.16)
project(sig_gen C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

if(WIN32)
    add_executable(sig_gen
        src/main.c
        src/renderer.c
        src/signal.c
        src/font_embedded.c
    )
    target_include_directories(sig_gen PRIVATE src)
else()
    add_executable(sig_gen
        src/main.c
        src/renderer.c
        src/signal.c
        src/font_embedded.c
    )
endif()

target_compile_options(sig_gen PRIVATE -Wall -Wextra)

target_link_libraries(sig_gen
    SDL2::SDL2
    SDL2_ttf::SDL2_ttf
)

if(WIN32)
    target_link_libraries(sig_gen SDL2::SDL2main)
    target_link_options(sig_gen PRIVATE -mwindows)
    add_custom_command(TARGET sig_gen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE:SDL2_ttf::SDL2_ttf>
            $<TARGET_FILE_DIR:sig_gen>
    )
else()
    target_link_libraries(sig_gen m)
endif()