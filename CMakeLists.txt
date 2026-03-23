cmake_minimum_required(VERSION 3.10)
project(aosh VERSION 0.1.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(
    ${CMAKE_SOURCE_DIR}/include/aosh/version.h.in
    ${CMAKE_BINARY_DIR}/generated/aosh/version.h
)

add_executable(aosh
    src/main.cpp
    src/shell.cpp
    src/parser.cpp
    src/color.cpp
    src/commands/registry.cpp
    src/commands/echo.cpp
    src/commands/filesystem.cpp
    src/commands/help.cpp
    src/commands/ls.cpp
    src/process.cpp
    src/completion/completer.cpp
    src/completion/completion_context.cpp
    src/completion/command_provider.cpp
    src/completion/path_exe_provider.cpp
    src/completion/filesystem_provider.cpp
)

target_include_directories(aosh PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)
