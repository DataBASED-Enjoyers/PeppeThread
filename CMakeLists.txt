# Указываем минимальную версию CMake
cmake_minimum_required(VERSION 3.10)

# Название проекта
project(TemperatureDistribution)

# Указываем стандарт языка C
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Проверяем поддержку OpenMP
find_package(OpenMP REQUIRED)


# Добавляем исполняемый файл
add_executable(openMP_task 
    src/task2/openMP_task.c)

# Добавляем флаги компиляции для OpenMP
if(OpenMP_C_FOUND)
    target_link_libraries(openMP_task PUBLIC OpenMP::OpenMP_C m) # Линкуем OpenMP и библиотеку math
    target_compile_options(openMP_task PUBLIC ${OpenMP_C_FLAGS})
else()
    target_link_libraries(openMP_task PUBLIC m) # Линкуем только библиотеку math, если OpenMP не найден
endif()

# Указываем дополнительные флаги компиляции
target_compile_options(openMP_task PRIVATE -Wall -O3)

# Выводим сообщение при успешной конфигурации
message(STATUS "Project configured. Run 'make' to build the program.")
