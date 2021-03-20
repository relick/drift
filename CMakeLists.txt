# CMakeList.txt : CMake project for drift, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)
cmake_policy(SET CMP0091 NEW)

project ("drift")

set (CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

## Set C++ details
set (CMAKE_CXX_STANDARD 20)
if (MSVC)
	set (CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /W3 /GR- /EHsc")
	if (CXX_COMPILER_ID MATCHES Clang)
		set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-m64 -fdiagnostics-absolute-paths")
	endif ()
	set (CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1 /DDEBUG=1")
	set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /Zi /O2 /Ob1 /DDEBUG=1")
	set (CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG=1")
else ()
	message(FATAL_ERROR "Only MSVC/clang-cl supported atm.")
endif ()

## Compile shaders
list (APPEND Shaders main render_target_to_screen depth_only skybox sprites)

if (MSVC)
	set (SOKOL_SDHC_COMPILER "msvc")
else ()
	set (SOKOL_SDHC_COMPILER "gcc")
endif ()

set (TOOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tools")
set (SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders")
set (SHADERS_COMPILED "")

foreach (Shader IN LISTS Shaders)
	add_custom_command (
		OUTPUT ${SHADERS_DIR}/${Shader}.h
		COMMAND ./sokol-shdc --input ${SHADERS_DIR}/${Shader}.glsl --output ${SHADERS_DIR}/${Shader}.h --slang glsl330:hlsl5 --errfmt=${SOKOL_SDHC_COMPILER} --genver=2
		DEPENDS ${SHADERS_DIR}/${Shader}.glsl ${SHADERS_DIR}/${Shader}.vert ${SHADERS_DIR}/${Shader}.frag
		WORKING_DIRECTORY ${TOOLS_DIR}
	)
	set (SHADERS_COMPILED ${SHADERS_COMPILED} "${SHADERS_DIR}/${Shader}.h")
endforeach ()

## A global compiler option worth having (fixes abseil)
add_compile_options ("$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")

## Setup external libraries
find_package (Bullet CONFIG REQUIRED)
find_package (Assimp CONFIG REQUIRED)
add_subdirectory (abseil-cpp)
add_subdirectory (external-cmake)

# Add source to this project's executable.
set (SOURCE_H "src/SystemOrdering.h" "src/systems/Core/RenderSystems.h"  "src/systems/Core/ImGuiSystems.h" "src/components/Core/FrameComponents.h" "src/systems/Core/TextAndGLDebugSystems.h" "src/CubeTest.h" "src/components/Core/CameraComponents.h" "src/managers/InputManager.h" "src/Entity.h" "src/systems/Core/PhysicsSystems.h" "src/managers/ResourceManager.h" "src/ID.h" "src/components/Game/PlayerComponents.h" "src/systems/Game/PlayerSystems.h" "src/managers/RenderManager.h" "src/managers/RenderTools/Pipeline.h" "src/managers/RenderTools/Enums.h" "src/managers/SoundManager.h" "src/systems/Core/SoundSystems.h" "src/components/Core/SoundComponents.h" "src/managers/ResourceIDs.h"  "src/common/fTrans.h" "src/common/Colour.h" "src/common/Debug.h" "src/common/MathDefs.h" "src/components/Game/UIComponents.h" "src/components/Core/ResourceComponents.h" "src/systems/Game/UISystems.h" "src/systems/Core/ResourceSystems.h")
set (SOURCE_CPP "src/drift.cpp" "src/managers/EntityManager.cpp" "src/systems/Core/ImGuiSystems.cpp" "src/systems/Core/TextAndGLDebugSystems.cpp" "src/CubeTest.cpp" "src/managers/InputManager.cpp" "src/systems/Core/PhysicsSystems.cpp" "src/components/Core/PhysicsComponents.cpp" "src/managers/ResourceManager.cpp" "src/systems/Core/RenderSystems.cpp" "src/components/Core/RenderComponents.cpp" "src/systems/Game/PlayerSystems.cpp" "src/managers/RenderManager.cpp" "src/stbImpl.cpp" "src/managers/SoundManager.cpp" "src/systems/Core/SoundSystems.cpp" "src/components/Core/SoundComponents.cpp" "src/common/Debug.cpp" "src/systems/Game/UISystems.cpp" "src/systems/Core/ResourceSystems.cpp")

add_executable (drift ${SOURCE_H} ${SOURCE_CPP} ${SHADERS_COMPILED})

target_include_directories (drift PUBLIC "src" "ecs/include" "ecs/tls/include" "sokol" "fontstash/src" "stb" "glm" "gcem/include" ${BULLET_INCLUDE_DIR})
target_link_libraries (drift PRIVATE ${BULLET_LIBRARIES} assimp::assimp absl::flat_hash_map absl::flat_hash_set absl::inlined_vector absl::str_format SoLoud ImGui)


## Compiler options
target_compile_options (drift PRIVATE
	$<$<CXX_COMPILER_ID:Clang,GNU>:
		-Wno-unused-function
	>
	$<$<CXX_COMPILER_ID:MSVC>:
		/Zc:preprocessor /utf-8
	>
)

## Set sokol_gfx API
## Define DEBUG_TOOLS on debug builds
## Also:
## so there's this absolute nonsense where one of the headers in windows will generate min and max macros.
## and this define stops it doing that because WHY THE HELL WOULD ANYONE DO THAT
## also disable the C security warnings since it's all in libraries I don't write
target_compile_definitions (drift PRIVATE
	$<$<BOOL:${USE_D3D11}>:
		SOKOL_D3D11
	>
	$<$<BOOL:${USE_OPENGL330}>:
		SOKOL_GLCORE33
	>
	$<$<CONFIG:Debug,RelWithDebInfo>:
		DEBUG_TOOLS
	>

	_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	NOMINMAX _CRT_SECURE_NO_WARNINGS
)