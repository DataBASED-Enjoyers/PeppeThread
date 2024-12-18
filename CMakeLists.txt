cmake_minimum_required(VERSION 3.18)
project(CUDA_LAB LANGUAGES CXX CUDA)

set(CMAKE_CUDA_STANDARD 11)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

find_package(CUDA REQUIRED)
find_package(MPI REQUIRED)

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

target_include_directories(n_body_cuda PRIVATE ${MPI_INCLUDE_PATH})
target_link_libraries(n_body_cuda PRIVATE MPI::MPI_CXX)