cmake_minimum_required(VERSION 3.5.0)
project(PeppeThread VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fopenmp")

find_package(MPI)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})

add_executable(
    task_1_general
    src/task1/general.c
)

add_executable(
    task_1_euler
    src/task1/euler.c
)

add_executable(
    task_2
    src/task2/dirichle.c
)

target_link_libraries(task_1_general ${MPI_C_LIBRARIES})
target_link_libraries(task_1_euler ${MPI_C_LIBRARIES})
target_link_libraries(task_2 ${MPI_C_LIBRARIES})