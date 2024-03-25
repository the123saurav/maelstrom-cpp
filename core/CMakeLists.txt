# Collect all source files recursively, need cmake configure for changes
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create a static library called 'core'
add_library(core STATIC ${SOURCES})

message(STATUS "Value of CMAKE_CURRENT_SOURCE_DIR is: ${CMAKE_CURRENT_SOURCE_DIR}")

# Public headers of the library (-I)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)