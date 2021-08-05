cmake_minimum_required(VERSION 3.16)

# set the project name
project(mp3)
set (DCMAKE_CXX_FLAGS -Werror -Wall -Wextra)
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

# build sketch as executable  
add_executable (mp3 desktop_mp3.cpp )
target_compile_options(mp3 PRIVATE -DEMULATOR)

# specify libraries
target_link_libraries(mp3 arduino_helix)

