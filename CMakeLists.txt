cmake_minimum_required(VERSION 3.10)
project(aosh VERSION 0.1.0 LANGUAGES CXX)

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
)

target_include_directories(aosh PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)
