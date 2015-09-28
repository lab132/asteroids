include(kr_set_pch)
include(kr_mirror_source_tree)

# Source Files
# ============
file(GLOB_RECURSE SOURCES *.h *.inl *.cpp)

# Target Setup
# ============
add_executable(asteroids ${SOURCES})
target_include_directories(asteroids PUBLIC ../../code)
target_include_directories(asteroids PUBLIC ..)
kr_set_pch(asteroids "pch.h")
kr_mirror_source_tree("${CMAKE_CURRENT_LIST_DIR}" ${SOURCES})

# Dependencies
# ============
find_package(OpenGL REQUIRED)
if(OPENGL_INCLUDE_DIR)
  # Needed on non-windows platforms (I think).
  target_include_directories(asteroids PUBLIC ${OPENGL_INCLUDE_DIR})
endif()

target_link_libraries(asteroids
                      krEngine
                      ${OPENGL_LIBRARIES})
