# CMakeList.txt : CMake project for drift, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project ("drift")

set (CMAKE_CXX_STANDARD 20)
if (MSVC)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:module")
endif()

find_package(Bullet CONFIG REQUIRED)
include_directories(${BULLET_INCLUDE_DIR})

include_directories("src" "ecs/include" "ecs/tls/include" "sokol" "imgui")

# Add source to this project's executable.
add_executable (drift "imgui/imgui.cpp" "imgui/imgui_demo.cpp" "imgui/imgui_draw.cpp" "imgui/imgui_widgets.cpp" "src/drift.cpp" "src/managers/EntityManager.cpp" "src/systems/SystemOrdering.h" "src/HandmadeMath.h" "src/components/Core/Render.h" "src/CubeTest.h" "src/systems/ImGui.h" "src/systems/ImGui.cpp" "src/components/Core/Frame.h")

target_link_libraries(drift PRIVATE LinearMath Bullet3Common BulletDynamics BulletSoftBody)
# TODO: Add tests and install targets if needed.
