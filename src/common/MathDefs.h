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

#include <concepts>

template< typename T >
concept ScalarType = std::integral< T > || std::floating_point< T >;

// Define some maths types
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/ext/matrix_common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

inline constexpr glm::qualifier c_glmQualifier = glm::defaultp;

template< ScalarType T > using Vector2Base = glm::tvec2< T, c_glmQualifier >;
template< ScalarType T > using Vector3Base = glm::tvec3< T, c_glmQualifier >;
template< ScalarType T > using Vector4Base = glm::tvec4< T, c_glmQualifier >;
template< ScalarType T > using Matrix2Base = glm::tmat2x2< T, c_glmQualifier >;
template< ScalarType T > using Matrix3Base = glm::tmat3x3< T, c_glmQualifier >;
template< ScalarType T > using Matrix4Base = glm::tmat4x4< T, c_glmQualifier >;
template< ScalarType T > using QuaternionBase = glm::tquat< T, c_glmQualifier >;

using Vec1 = float;
using Vec2 = Vector2Base< float >;
using Vec3 = Vector3Base< float >;
using Vec4 = Vector4Base< float >;
using Mat2 = Matrix2Base< float >;
using Mat3 = Matrix3Base< float >;
using Mat4 = Matrix4Base< float >;
using Quat = QuaternionBase< float >;

using dVec1 = double;
using dVec2 = Vector2Base< double >;
using dVec3 = Vector3Base< double >;
using dVec4 = Vector4Base< double >;
using dMat2 = Matrix2Base< double >;
using dMat3 = Matrix3Base< double >;
using dMat4 = Matrix4Base< double >;
using dQuat = QuaternionBase< double >;

using iVec1 = int32;
using iVec2 = Vector2Base< int32 >;
using iVec3 = Vector3Base< int32 >;
using iVec4 = Vector4Base< int32 >;
using iMat2 = Matrix2Base< int32 >;
using iMat3 = Matrix3Base< int32 >;
using iMat4 = Matrix4Base< int32 >;

template< typename T >
constexpr T Identity()
{
	return glm::identity< T >();
}

template< typename T >
constexpr T Normalise(T const& _t)
{
	return glm::normalize( _t );
}
