cmake_minimum_required(VERSION 3.16)

project(lock_free_queues LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bench
        src/main.cpp
)

target_include_directories(bench
        PRIVATE
        include
        benchmark
)

target_compile_definitions(bench
        PRIVATE
        PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_options(bench PRIVATE
        -O3
        -march=native
)

target_link_libraries(bench PRIVATE pthread)