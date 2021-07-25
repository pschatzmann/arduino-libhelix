cmake_minimum_required(VERSION 3.16)

# set the project name
project(mp3)
set (DCMAKE_CXX_FLAGS -Werror -Wall -Wextra)

# Collect all relevant source files
file(GLOB_RECURSE SRC_LIST CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.c" )

# build sketch as executable  
add_executable (mp3 desktop_mp3.cpp ${SRC_LIST})
target_compile_options(mp3 PRIVATE -DUSE_DEFAULT_STDLIB -DDEFINE_MAIN -DEMULATOR)

# specify libraries
target_link_libraries(mp3 arduino_helix)

