# CMake requires us to declare the minimum (and optionally maximum)
# versions we support before we do anything else
cmake_minimum_required(VERSION 3.20)

# For our purposes, everything in CMake must belong to a project
# Fun Note: Strings are (usually) implicit
project(lab3)

# 🫣
include(cmake/DontLook.cmake)

add_subdirectory(coolrt)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(test)
