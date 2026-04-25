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

# Testing configuration
enable_testing()

# Core test sources
set(TEST_SOURCES
    parser/http_parser.c
    parser/url.c
    response/response_builder.c
    router/handlers.c
    router/router.c
    socket/connection.c
    tests/unity/unity.c
)

# Create test executables
add_executable(parser_test tests/parser_test.c ${TEST_SOURCES})
target_include_directories(parser_test PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/tests/unity)

add_executable(response_test tests/response_test.c ${TEST_SOURCES})
target_include_directories(response_test PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/tests/unity)

add_executable(router_test tests/router_test.c ${TEST_SOURCES})
target_include_directories(router_test PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/tests/unity)

add_executable(connection_test tests/connection_test.c ${TEST_SOURCES})
target_include_directories(connection_test PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/tests/unity)

# Register tests
add_test(NAME ParserTest COMMAND parser_test)
add_test(NAME ResponseTest COMMAND response_test)
add_test(NAME RouterTest COMMAND router_test)
add_test(NAME ConnectionTest COMMAND connection_test)