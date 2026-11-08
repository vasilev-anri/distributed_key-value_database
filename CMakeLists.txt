
cmake_minimum_required(VERSION 4.3)
project(k_v_db LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─────────────────────────── Sanitizers ───────────────────────────
option(KVDB_SANITIZE "Build with AddressSanitizer + UBSan" OFF)

if(KVDB_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# ─────────────────────────── Engine core ──────────────────────────
add_library(kvdb_core INTERFACE)

target_include_directories(kvdb_core INTERFACE
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_options(kvdb_core INTERFACE
        -Wall -Wextra -Wpedantic
)

# ─────────────────────────── Main executable ──────────────────────
add_executable(k_v_db
        main.cpp
        src/reactor/reactor.cpp
        src/reactor/reactor.h
        src/handlers/event_handler.h
        src/handlers/udp_handler.cpp
        src/handlers/udp_handler.h
)

target_link_libraries(k_v_db PRIVATE kvdb_core)

# ─────────────────────────── GoogleTest ────────────────────────────
include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
)

FetchContent_MakeAvailable(googletest)

# ─────────────────────────── Unit tests ───────────────────────────
enable_testing()

add_executable(unit_tests
        tests/unique_fd_test.cpp
)

target_link_libraries(unit_tests PRIVATE
        kvdb_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
