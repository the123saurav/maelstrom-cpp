# Create a binary called 'echo'
add_executable(echo src/echo.cpp)

# Link the 'core' static library with 'echo'
target_link_libraries(echo PRIVATE core)