cmake_minimum_required(VERSION 3.10)
project(maelstrom-cpp)

# Specify C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include the subdirectories
add_subdirectory(core)
add_subdirectory(echo)
