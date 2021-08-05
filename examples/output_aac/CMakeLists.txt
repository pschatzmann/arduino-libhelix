cmake_minimum_required(VERSION 3.16)

# set the project name
project(aac)
set (DCMAKE_CXX_FLAGS -Werror -Wall -Wextra)
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

# build sketch as executable  
add_executable (aac desktop_aac.cpp )
target_compile_options(aac PRIVATE -DEMULATOR)

# specify libraries
target_link_libraries(aac arduino_helix)
