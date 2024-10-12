cmake_minimum_required(VERSION 3.5.0)
project(PeppeThread VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
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
    src/task1/pi.c
)

add_executable(
    mandelbrot
    src/task2/mandelbrot.c
)
target_link_libraries(mandelbrot m)

add_executable(
    rwlock_list
    src/task3/pth_ll_rwl.c
)

add_executable(
    ppt_rwlock_list
    src/task3/rw_lock.c
    src/task3/ppt_ll_rwl.c
)

target_include_directories(pi PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(mandelbrot PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_include_directories(rwlock_list PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(ppt_rwlock_list PUBLIC ${CMAKE_SOURCE_DIR}/include)