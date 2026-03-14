cmake_minimum_required(VERSION 3.16)

project(lock_free_spsc_queue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(benchmark
        src/main.cpp
)

target_include_directories(benchmark
        PRIVATE
        include
)

target_compile_options(benchmark PRIVATE
        -O3
        -march=native
        -pthread
)

target_link_libraries(benchmark PRIVATE pthread)