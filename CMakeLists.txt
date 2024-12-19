cmake_minimum_required(VERSION 3.10)
project(CUDA_LAB LANGUAGES CXX CUDA)

project(Lab3)

set(CMAKE_CUDA_STANDARD 11)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(CMAKE_C_STANDARD_REQUIRED True)

find_package(OpenMP REQUIRED)
find_package(CUDA REQUIRED)
find_package(MPI REQUIRED)


add_executable(openMP_task 
    src/task2/openMP_task.c
)
add_executable(n_body_cuda
    src/task1/n_body_cuda.cu
)
add_executable(check_cuda_size
    utils/check_cuda_size.cu
)

set_target_properties(n_body_cuda PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "61;75;86;89"          # 61 -- Pascal, 75 -- Turing, 86 -- Ampere, 89 -- Ada Lovelace
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/task1
)

set_target_properties(check_cuda_size PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "61;75;86;89"          # 61 -- Pascal, 75 -- Turing, 86 -- Ampere, 89 -- Ada Lovelace
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

if(OpenMP_C_FOUND)
    target_link_libraries(openMP_task PUBLIC OpenMP::OpenMP_C m)
    target_compile_options(openMP_task PUBLIC ${OpenMP_C_FLAGS})
else()
    target_link_libraries(openMP_task PUBLIC m)
endif()

target_compile_options(openMP_task PRIVATE -Wall -O3)
target_include_directories(n_body_cuda PRIVATE ${MPI_INCLUDE_PATH})
target_link_libraries(n_body_cuda PRIVATE MPI::MPI_CXX)

message(STATUS "Project configured. Run 'make' to build the program.")
