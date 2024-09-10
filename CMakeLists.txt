cmake_minimum_required(VERSION 3.16)

if (DEFINED ESP_PLATFORM)
  # idf component
  idf_component_register(
      SRC_DIRS src src/utils src/libhelix-aac src/libhelix-mp3
      INCLUDE_DIRS src src/utils src/libhelix-aac src/libhelix-mp3
      REQUIRES arduino-esp32
  )

  target_compile_options(${COMPONENT_LIB} INTERFACE -Wno-error -Wno-format)
  target_compile_options(${COMPONENT_LIB} PRIVATE -DUSE_DEFAULT_STDLIB)
  add_compile_definitions(ESP32)
else()

  # set the project name
  project(arduino_helix)

  # lots of warnings and all warnings as errors
  ## add_compile_options(-Wall -Wextra )
  set(CMAKE_CXX_STANDARD 17)

  option(MP3_EXAMPLES "build examples" OFF)

  file(GLOB_RECURSE SRC_LIST_C CONFIGURE_DEPENDS  "${PROJECT_SOURCE_DIR}/src/*.c" )
  file(GLOB_RECURSE SRC_LIST_CPP CONFIGURE_DEPENDS  "${PROJECT_SOURCE_DIR}/src/*.cpp" )

  # define libraries
  add_library (arduino_helix ${SRC_LIST_C} ${SRC_LIST_CPP})

  # prevent compile errors
  target_compile_options(arduino_helix PRIVATE -DUSE_DEFAULT_STDLIB)

  # define location for header files
  target_include_directories(arduino_helix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/libhelix-mp3 ${CMAKE_CURRENT_SOURCE_DIR}/src/libhelix-aac )

  # build examples
  if(MP3_EXAMPLES)
    add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/examples/output_mp3")
    add_subdirectory( "${CMAKE_CURRENT_SOURCE_DIR}/examples/output_aac")
  endif()
endif()