# SETUP
cmake_minimum_required(VERSION 3.10)
project(ClairDMG)

# Set C Standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Get SDL2 library 
find_package(SDL2 REQUIRED)

# Include paths
include_directories(${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS})

# Get Library Source Files
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.c)

# Main Executable
add_executable(clair-dmg ${SOURCES})

# Link libraries
target_link_libraries(clair-dmg ${SDL2_LIBRARIES})
