file(GLOB_RECURSE SOURCES "src/*.cpp")


# Create a binary called 'broadcast'
add_executable(broadcast ${SOURCES})

target_include_directories(broadcast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link the 'core' static library with 'broadcast'
target_link_libraries(broadcast PRIVATE core)