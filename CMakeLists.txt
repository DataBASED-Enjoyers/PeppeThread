cmake_minimum_required(VERSION 3.5.0)
project(PeppeThread VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fopenmp")

find_package(MPI)
include_directories(SYSTEM ${MPI_INCLUDE_PATH})

add_executable(
    task_1_row
    src/task1/matmul_row.c
)

add_executable(
    task_1_col
    src/task1/matmul_col.c
)

add_executable(
    task_1_chess
    src/task1/matmul_chess.c
)

add_executable(
    task_2
    src/task2/cannon_matmul.c
)

target_link_libraries(task_1_row ${MPI_C_LIBRARIES})
target_link_libraries(task_1_col ${MPI_C_LIBRARIES})
target_link_libraries(task_1_chess ${MPI_C_LIBRARIES})
target_link_libraries(task_2 ${MPI_C_LIBRARIES})