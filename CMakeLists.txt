cmake_minimum_required(VERSION 3.5.0)
project(PeppeThread VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fopenmp")

find_package(MPI)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})

add_executable(
    main
    src/main.c
)

target_link_libraries(main ${MPI_C_LIBRARIES})