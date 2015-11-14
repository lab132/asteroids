
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

# Name of the entire project.
project("asteroids")

find_path(KREPEL_DIR lib/krEngine.lib
          PATHS "$ENV{KREPEL_DIR}" "C:/Program Files/krepel" "$ENV{PATH}"
          DOC "Path to the krepel installation."
          NO_DEFAULT_PATH)

if(NOT EXISTS "${KREPEL_DIR}")
  message(FATAL_ERROR "Please specify the path to the krepel installation.")
endif()

set(CMAKE_MODULE_PATH "${ASTEROIDS_DIR}/build/CMake" "${KREPEL_DIR}/build/CMake")
include(kr_setup)
kr_setup()

include(platforms/${CMAKE_SYSTEM_NAME})
include(targets/glew)
include(targets/ezEngine)
include(targets/krepel)


set(ASTEROIDS_DIR "${CMAKE_SOURCE_DIR}" CACHE PATH "Root directory of asteroids.")
mark_as_advanced(ASTEROIDS_DIR)

include_directories("code")
add_subdirectory("code")

include(kr_create_target_pullRuntimeBinaries)
kr_create_target_pullRuntimeBinaries("${ASTEROIDS_DIR}/bin" "${KREPEL_DIR}/bin/")
