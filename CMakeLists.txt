cmake_minimum_required(VERSION 3.16)

project(pi_atecc VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(pi_atecc
    src/pi_atecc.c
)

target_include_directories(pi_atecc PRIVATE src)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(pi_atecc PRIVATE -Wall -Wextra -Wpedantic)
endif()
