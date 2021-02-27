# CMakeList.txt : CMake project for drift, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project ("drift")

## Compile shaders
list (APPEND Shaders phong unlit main render_target_to_screen depth_only)

if(MSVC)
	set (SOKOL_SDHC_COMPILER "msvc")
else()
	set (SOKOL_SDHC_COMPILER "gcc")
endif()

set (TOOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tools")
set (SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders")
set (SHADERS_COMPILED "")

foreach(Shader IN LISTS Shaders)
	add_custom_command(
		OUTPUT ${SHADERS_DIR}/${Shader}.h
		COMMAND ./sokol-shdc --input ${SHADERS_DIR}/${Shader}.glsl --output ${SHADERS_DIR}/${Shader}.h --slang glsl330:hlsl5 --errfmt=${SOKOL_SDHC_COMPILER} --genver=2
		DEPENDS ${SHADERS_DIR}/${Shader}.glsl ${SHADERS_DIR}/${Shader}.vert ${SHADERS_DIR}/${Shader}.frag
		WORKING_DIRECTORY ${TOOLS_DIR}
	)
	set (SHADERS_COMPILED ${SHADERS_COMPILED} "${SHADERS_DIR}/${Shader}.h")
endforeach()


## Set C++ version and enable features
set (CMAKE_CXX_STANDARD 20)
if (MSVC)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:preprocessor /utf-8")
endif()

## Set sokol_gfx API
if(USE_D3D11)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSOKOL_D3D11")
elseif(USE_OPENGL330)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSOKOL_GLCORE33")
endif()

## so there's this absolute nonsense where one of the headers in windows will generate min and max macros.
## and this define stops it doing that because WHY THE HELL WOULD ANYONE DO THAT
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX")


## Define DEBUG_TOOLS on debug builds for all platforms
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG_TOOLS")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DDEBUG_TOOLS")


## Setup libraries and includes
find_package(Bullet CONFIG REQUIRED)
include_directories(${BULLET_INCLUDE_DIR})

find_package(Assimp CONFIG REQUIRED)
include_directories(assimp::assimp)

link_directories("lib")

include_directories("src" "ecs/include" "ecs/tls/include" "sokol" "imgui" "fontstash/src" "stb" "glm" "gcem/include" "soloud/include")

# Add source to this project's executable.
set (SOURCE_H "src/systems/Core/SystemOrdering.h" "src/systems/Core/RenderSystems.h"  "src/systems/Core/ImGuiSystems.h" "src/components/Core/FrameComponents.h" "src/systems/Core/TextAndGLDebugSystems.h" "src/CubeTest.h" "src/components/Core/CameraComponents.h" "src/managers/InputManager.h" "src/Entity.h" "src/systems/Core/PhysicsSystems.h" "src/managers/ResourceManager.h" "src/ID.h" "src/components/Game/PlayerComponents.h" "src/systems/Game/PlayerSystems.h" "src/managers/RenderManager.h" "src/managers/RenderTools/Pipeline.h" "src/managers/RenderTools/Enums.h" "src/managers/SoundManager.h" "src/systems/Core/SoundSystems.h" "src/components/Core/SoundComponents.h" "src/managers/ResourceIDs.h" )
set (SOURCE_CPP "imgui/imgui.cpp" "imgui/imgui_demo.cpp" "imgui/imgui_tables.cpp" "imgui/imgui_draw.cpp" "imgui/imgui_widgets.cpp" "src/drift.cpp" "src/managers/EntityManager.cpp" "src/systems/Core/ImGuiSystems.cpp" "src/systems/Core/TextAndGLDebugSystems.cpp" "src/CubeTest.cpp" "src/managers/InputManager.cpp" "src/systems/Core/PhysicsSystems.cpp" "src/components/Core/PhysicsComponents.cpp" "src/managers/ResourceManager.cpp" "src/systems/Core/RenderSystems.cpp" "src/components/Core/RenderComponents.cpp" "src/systems/Game/PlayerSystems.cpp" "src/managers/RenderManager.cpp" "src/stbImpl.cpp" "src/managers/SoundManager.cpp" "src/systems/Core/SoundSystems.cpp" "src/components/Core/SoundComponents.cpp")

add_executable (drift ${SOURCE_H} ${SOURCE_CPP} ${SHADERS_COMPILED})

target_link_libraries(drift PRIVATE ${BULLET_LIBRARIES} assimp::assimp "soloud_static_x64")