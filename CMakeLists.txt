# CMakeList.txt : CMake project for drift, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project ("drift")

set (CMAKE_CXX_STANDARD 20)

find_package(Bullet CONFIG REQUIRED)
include_directories(${BULLET_INCLUDE_DIR})

include_directories("ecs/include" "ecs/tls/include" "sokol")

# Add source to this project's executable.
add_executable (drift "src/drift.cpp"  "src/components.h" "src/systems.h" "src/common.h" "src/components/transform.h")

target_link_libraries(drift PRIVATE LinearMath Bullet3Common BulletDynamics BulletSoftBody)
# TODO: Add tests and install targets if needed.
