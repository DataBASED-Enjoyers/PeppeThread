cmake_minimum_required(VERSION 3.5.0)
project(PeppeThread VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lm -Wall")

add_executable(
    pi 
    src/pi.c
)

add_executable(
    mandelbrot
    src/mandelbrot.c
)

target_link_libraries(mandelbrot m)
