#pragma once

// Define some number types
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "gcem.hpp"
using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;
using isize = std::ptrdiff_t;
using usize = std::size_t;
using wchar = wchar_t;

// Define some maths types
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/ext/matrix_common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

using fVec2 = glm::vec2;
using fVec3 = glm::vec3;
using fVec4 = glm::vec4;
using fMat2 = glm::mat2;
using fMat3 = glm::mat3;
using fMat4 = glm::mat4;
using fQuat = glm::quat;

using iVec2 = glm::ivec2;

constexpr fQuat fQuatIdentity()
{
	return glm::identity<fQuat>();
}