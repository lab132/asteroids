include(kr_set_pch)
include(kr_mirror_source_tree)
include(kr_target_make_launchable)

# Source Files
# ============
file(GLOB_RECURSE SOURCES *.h *.inl *.cpp)

# Target Setup
# ============
add_library(asteroids SHARED ${SOURCES})
target_include_directories(asteroids PUBLIC ../../code)
target_include_directories(asteroids PUBLIC ..)
kr_set_pch(asteroids "pch.h")
kr_mirror_source_tree("${CMAKE_CURRENT_LIST_DIR}" ${SOURCES})

target_link_libraries(asteroids krEngine)

kr_target_make_launchable(asteroids)
