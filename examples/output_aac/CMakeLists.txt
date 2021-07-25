cmake_minimum_required(VERSION 3.20)

# set the project name
project(aac)
set (DCMAKE_CXX_FLAGS -Werror -Wall -Wextra)

# Collect all relevant source files
file(GLOB_RECURSE SRC_LIST CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.c" )

# build sketch as executable  
add_executable (aac desktop_aac.cpp ${SRC_LIST})
target_compile_options(aac PRIVATE -DUSE_DEFAULT_STDLIB -DDEFINE_MAIN -DEMULATOR)

# specify libraries
target_link_libraries(aac arduino_helix)

