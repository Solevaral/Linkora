cmake_minimum_required(VERSION 3.20)
project(Linkora VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LINKORA_BUILD_TESTS "Build Linkora tests" ON)
option(LINKORA_BUILD_GUI "Build Linkora Qt GUI" ON)

find_package(OpenSSL QUIET)
if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR
        "OpenSSL not found. Install OpenSSL development package first. "
        "Linux example: sudo apt install libssl-dev")
endif()

set(LINKORA_CORE_SOURCES
    src/core/crypto.cpp
    src/core/password.cpp
)

if(WIN32)
    list(APPEND LINKORA_CORE_SOURCES src/core/tunnel_windows.cpp)
else()
    list(APPEND LINKORA_CORE_SOURCES src/core/tunnel_linux.cpp)
endif()

set(LINKORA_NETWORK_SOURCES
    src/network/transport.cpp
    src/network/frame.cpp
)

if(WIN32)
    list(APPEND LINKORA_NETWORK_SOURCES src/network/route_manager_windows.cpp)
else()
    list(APPEND LINKORA_NETWORK_SOURCES src/network/route_manager_linux.cpp)
endif()

set(LINKORA_APP_SOURCES
    src/app/auth.cpp
    src/app/coordinator_api.cpp
    src/app/config.cpp
    src/utils/logging.cpp
)

add_library(linkora_core STATIC ${LINKORA_CORE_SOURCES})
target_include_directories(linkora_core PUBLIC src)
target_link_libraries(linkora_core PUBLIC OpenSSL::Crypto)

if(MSVC)
    target_compile_options(linkora_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(linkora_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(linkora_host src/app/host_main.cpp ${LINKORA_NETWORK_SOURCES} ${LINKORA_APP_SOURCES})
target_include_directories(linkora_host PRIVATE src)
target_link_libraries(linkora_host PRIVATE linkora_core)

add_executable(linkora_client_worker src/app/client_main.cpp ${LINKORA_NETWORK_SOURCES} ${LINKORA_APP_SOURCES})
target_include_directories(linkora_client_worker PRIVATE src)
target_link_libraries(linkora_client_worker PRIVATE linkora_core)

add_executable(linkora_hash src/app/hash_main.cpp)
target_include_directories(linkora_hash PRIVATE src)
target_link_libraries(linkora_hash PRIVATE linkora_core)

if(LINKORA_BUILD_GUI)
    find_package(Qt6 COMPONENTS Widgets QUIET)
    if(Qt6_FOUND)
        add_executable(linkora_gui
            src/gui/gui_main.cpp
            src/gui/main_window.cpp
            src/gui/main_window.h
        )
        set_target_properties(linkora_gui PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
        target_include_directories(linkora_gui PRIVATE src)
        target_link_libraries(linkora_gui PRIVATE Qt6::Widgets)
    else()
        find_package(Qt5 COMPONENTS Widgets QUIET)
        if(Qt5_FOUND)
            add_executable(linkora_gui
                src/gui/gui_main.cpp
                src/gui/main_window.cpp
                src/gui/main_window.h
            )
            set_target_properties(linkora_gui PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
            target_include_directories(linkora_gui PRIVATE src)
            target_link_libraries(linkora_gui PRIVATE Qt5::Widgets)
        else()
            message(WARNING "Qt not found: linkora_gui target will not be built. Install Qt6/Qt5 Widgets and reconfigure.")
        endif()
    endif()
endif()

if(LINKORA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
