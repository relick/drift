# CMakeList.txt : CMake project for drift, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("drift")

set (CMAKE_CXX_STANDARD 20)

include_directories("ecs/include" "ecs/tls/include" "sokol")

# Add source to this project's executable.
add_executable (drift "src/drift.cpp" )

# TODO: Add tests and install targets if needed.
