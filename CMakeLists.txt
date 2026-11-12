cmake_minimum_required(VERSION 4.3)
project(k_v_db LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─────────────────────────── Sanitizers ───────────────────────────
# Before any target, so it applies to everything below.
option(KVDB_SANITIZE "Build with AddressSanitizer + UBSan" OFF)

if(KVDB_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# ─────────────────────────── Dependencies ─────────────────────────
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENDHT REQUIRED IMPORTED_TARGET opendht)

# ─────────────────────────── Engine core ──────────────────────────
# All engine code, compiled once; every executable links this.
add_library(kvdb_core STATIC
        src/reactor/reactor.cpp
        src/handlers/udp_handler.cpp
        src/handlers/tcp_listener.cpp
        src/handlers/tcp_connection.cpp
        src/handlers/timer_queue.cpp
        src/handlers/mailbox.cpp
        src/handlers/signal_handler.cpp
        src/dht/dht_node.cpp
        src/protocol/codec.cpp
)

target_include_directories(kvdb_core PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_options(kvdb_core PUBLIC
        -Wall -Wextra -Wpedantic
)

target_link_libraries(kvdb_core PUBLIC
        Threads::Threads
        PkgConfig::OPENDHT
)

# ─────────────────────────── Main executable ──────────────────────
add_executable(k_v_db
        main.cpp
)

target_link_libraries(k_v_db PRIVATE kvdb_core)

# ─────────────────────────── Tools ────────────────────────────────
add_executable(dht_spike
        tools/dht_spike.cpp
)

target_link_libraries(dht_spike PRIVATE kvdb_core)

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
        tests/protocol_test.cpp
)

target_link_libraries(unit_tests PRIVATE
        kvdb_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)