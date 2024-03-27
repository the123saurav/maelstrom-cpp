cmake_minimum_required(VERSION 3.5)
project(YourProjectName)

# List source files explicitly if possible
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Find Boost libraries, including the JSON component
find_package(Boost REQUIRED COMPONENTS system filesystem json)

# Create a static library called 'core'
add_library(core STATIC ${SOURCES})

# Link Boost libraries to the 'core' library
# If Boost JSON is a header-only library, it does not need to be explicitly linked
target_link_libraries(core PUBLIC ${Boost_LIBRARIES})

# Specify include directories for 'core' library
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# If Boost was found, include the directories for the 'core' library
if(Boost_FOUND)
    target_include_directories(core PUBLIC ${Boost_INCLUDE_DIRS})
endif()
