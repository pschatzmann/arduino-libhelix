cmake_minimum_required(VERSION 3.16)

# set the project name
project(arduino_helix)
include(FetchContent)

# lots of warnings and all warnings as errors
add_compile_options(-Wall -Wextra -pedantic )

# Build with Linux Arduino Emulator
FetchContent_Declare(arduino_emulator GIT_REPOSITORY "https://github.com/pschatzmann/Arduino-Emulator.git" GIT_TAG main )
FetchContent_GetProperties(arduino_emulator)
if(NOT arduino_emulator_POPULATED)
    FetchContent_Populate(arduino_emulator)
    add_subdirectory(${arduino_emulator_SOURCE_DIR})
endif()


file(GLOB_RECURSE SRC_LIST_C CONFIGURE_DEPENDS  "${PROJECT_SOURCE_DIR}/src/*.c" )

# define libraries
add_library (arduino_helix ${SRC_LIST_C})

# prevent compile errors
target_compile_options(arduino_helix PRIVATE -DUSE_DEFAULT_STDLIB)
target_compile_options(arduino_emulator PRIVATE -DUSE_DEFAULT_STDLIB -DEMULATOR )

# define location for header files
target_include_directories(arduino_helix  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/libhelix-mp3 ${CMAKE_CURRENT_SOURCE_DIR}/src/libhelix-aac)

# specify libraries
target_link_libraries(arduino_helix arduino_emulator)

# build examples
add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/examples/output_mp3/")
add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/examples/output_aac/")
