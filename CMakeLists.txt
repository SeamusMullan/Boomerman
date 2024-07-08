cmake_minimum_required(VERSION 3.28)
project(Boomerman C)

set(CMAKE_C_STANDARD 11)

# Find raylib
find_package(raylib REQUIRED)

# Add the executable
add_executable(Boomerman main.c)

# Link raylib
target_link_libraries(Boomerman raylib)
