cmake_minimum_required(VERSION 3.10)
project(WebServer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Compiler warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# Collect source files
file(GLOB_RECURSE SOURCES
    "socket/*.c"
    "parser/*.c"
    "response/*.c"
    "router/*.c"
)

add_executable(webserver main.c ${SOURCES})

target_include_directories(webserver PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Link socket library on Windows (no-op on Linux/macOS)
if(WIN32)
    target_link_libraries(webserver PRIVATE ws2_32)
endif()