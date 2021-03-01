## Used by sub-dirs to get to source location
set(SRC_DIR ${PROJECT_SOURCE_DIR})

## Dependencies that don't have properly generated CMakeLists go here, with custom CMakeLists
add_subdirectory(SoLoudStatic)
add_subdirectory(ImGui)