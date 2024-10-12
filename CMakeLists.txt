cmake_minimum_required(VERSION 3.5.0)
project(PeppeThread VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

find_program(CLANG_FORMAT NAMES clang-format)

if(CLANG_FORMAT)
    file(GLOB_RECURSE C_SOURCES *.c *.h)

    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT}
        -i
        -style=file
        ${C_SOURCES}
        COMMENT "Running clang-format on all source files"
    )
else()
    message(WARNING "clang-format not found!")
endif()


add_executable(
    pi 
    src/pi.c
)

add_executable(
    mandelbrot
    src/mandelbrot.c
)

target_link_libraries(mandelbrot m)
