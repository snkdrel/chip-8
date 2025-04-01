cmake_minimum_required(VERSION 3.16)
project(chip-8)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

add_executable(chip-8 WIN32 main.cpp)

target_link_libraries(chip-8 PRIVATE SDL3::SDL3)