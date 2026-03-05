cmake_minimum_required(VERSION 3.20)
project(Linkora VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LINKORA_BUILD_TESTS "Build Linkora tests" ON)

find_package(OpenSSL QUIET)
if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR
        "OpenSSL not found. Install OpenSSL development package first. "
        "Linux example: sudo apt install libssl-dev")
endif()

set(LINKORA_CORE_SOURCES
    src/core/crypto.cpp
)

if(WIN32)
    list(APPEND LINKORA_CORE_SOURCES src/core/tunnel_windows.cpp)
else()
    list(APPEND LINKORA_CORE_SOURCES src/core/tunnel_linux.cpp)
endif()

add_library(linkora_core STATIC ${LINKORA_CORE_SOURCES})
target_include_directories(linkora_core PUBLIC src)
target_link_libraries(linkora_core PUBLIC OpenSSL::Crypto)

if(MSVC)
    target_compile_options(linkora_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(linkora_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(linkora_host src/app/host_main.cpp src/network/transport.cpp)
target_include_directories(linkora_host PRIVATE src)
target_link_libraries(linkora_host PRIVATE linkora_core)

add_executable(linkora_client src/app/client_main.cpp src/network/transport.cpp)
target_include_directories(linkora_client PRIVATE src)
target_link_libraries(linkora_client PRIVATE linkora_core)

if(LINKORA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
