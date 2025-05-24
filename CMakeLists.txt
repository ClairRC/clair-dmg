# SETUP
cmake_minimum_required(VERSION 3.10)
project(GameBoyEmulatorIDKTheName)

# Set C Standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include path
include_directories(${CMAKE_SOURCE_DIR}/include)

# Get Library Source Files
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.c)
list(REMOVE_ITEM SOURCES ${CMAKE_SOURCE_DIR}/src/logging.c)
list(REMOVE_ITEM SOURCES ${CMAKE_SOURCE_DIR}/src/main.c)

# Create library for files for reuse with tests
add_library(emu_lib ${SOURCES})

# Main Executable
add_executable(main ${CMAKE_SOURCE_DIR}/src/main.c ${CMAKE_SOURCE_DIR}/src/logging.c)
target_link_libraries(main emu_lib)

# Unit Tests
file(GLOB_RECURSE TESTS ${CMAKE_SOURCE_DIR}/tests/*.c)

foreach(test ${TESTS})
    get_filename_component(test_name ${test} NAME_WE)
    add_executable(${test_name} ${test} ${CMAKE_SOURCE_DIR}/src/logging.c)
    target_link_libraries(${test_name} emu_lib)
    set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_BINARY_DIR}/tests)
    target_compile_definitions(${test_name} PUBLIC SUPPRESS_ERROR_OUTPUT)
endforeach()
