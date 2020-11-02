# CMakeList.txt : CMake project for drift, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project ("drift")

## Compile shaders
list (APPEND Shaders phong unlit)

if(MSVC)
	set (SOKOL_SDHC_COMPILER "msvc")
else()
	set (SOKOL_SDHC_COMPILER "gcc")
endif()

set (TOOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tools")
set (SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders")

foreach(Shader IN LISTS Shaders)
	add_custom_command(
		OUTPUT ${SHADERS_DIR}/${Shader}.h
		COMMAND sokol-shdc --input ${SHADERS_DIR}/${Shader}.glsl --output ${SHADERS_DIR}/${Shader}.h --slang glsl330:hlsl5 --errfmt=${SOKOL_SDHC_COMPILER} --genver=1 --ifdef
		DEPENDS ${SHADERS_DIR}/${Shader}.glsl
		WORKING_DIRECTORY ${TOOLS_DIR}
	)
endforeach()


## Set C++ version and enable features
set (CMAKE_CXX_STANDARD 20)
if (MSVC)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:module")
endif()

## Set sokol_gfx API
if(USE_D3D11)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSOKOL_D3D11")
elseif(USE_OPENGL330)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSOKOL_GLCORE33")
endif()


## Define DEBUG_TOOLS on debug builds for all platforms
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG_TOOLS")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DDEBUG_TOOLS")


## Setup libraries and includes
find_package(Bullet CONFIG REQUIRED)
include_directories(${BULLET_INCLUDE_DIR})

include_directories("src" "ecs/include" "ecs/tls/include" "sokol" "imgui" "fontstash/src")

# Add source to this project's executable.
add_executable (drift
"imgui/imgui.cpp"
"imgui/imgui_demo.cpp"
"imgui/imgui_draw.cpp"
"imgui/imgui_widgets.cpp"
"src/drift.cpp"
"src/managers/EntityManager.cpp"
"src/systems/SystemOrdering.h"
"src/HandmadeMath.h"
"src/components/Core/Render.h"
"src/CubeTest.h"
"src/systems/ImGui.h"
"src/systems/ImGui.cpp"
"src/components/Core/Frame.h"
"src/systems/TextAndGLDebug.h"
"src/systems/TextAndGLDebug.cpp"
"src/CubeTest2.h"
"src/CubeTest2.cpp"
"src/components/Core/Camera.h"
"src/managers/Input.h"
"src/managers/Input.cpp"
"src/Entity.h"
"src/shaders/phong.h"
"src/shaders/unlit.h"
)

target_link_libraries(drift PRIVATE LinearMath Bullet3Common BulletDynamics BulletSoftBody)
