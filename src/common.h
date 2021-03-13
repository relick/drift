#pragma once

#if DEBUG_TOOLS
	#define IMGUI_DEBUG_ENABLED 1
#endif

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
using fMat3 = glm::mat3;
using fMat4 = glm::mat4;
using fQuat = glm::quat;

constexpr fQuat fQuatIdentity()
{
	return glm::identity<fQuat>();
}

#include <LinearMath/btTransform.h>
struct fTrans
{
	fMat3 m_basis{1.0f};
	fVec3 m_origin{};

	fTrans operator*(fTrans const& _t) const
	{
		return fTrans{ m_basis * _t.m_basis,
			(*this)(_t.m_origin) };
	}

	fVec3 operator()(fVec3 const& _v) const
	{
		return fVec3(glm::dot(_v, m_basis[0]), glm::dot(_v, m_basis[1]), glm::dot(_v, m_basis[2])) + m_origin;
	}

	fTrans& operator*=(fTrans const& _t)
	{
		m_origin += m_basis * _t.m_origin;
		m_basis *= _t.m_basis;
		return *this;
	}

	fVec3 right() const
	{
		return glm::normalize(m_basis[0]);
	}
	fVec3 up() const
	{
		return glm::normalize(m_basis[1]);
	}
	fVec3 forward() const
	{
		return glm::normalize(m_basis[2]);
	}

	fMat4 GetRenderMatrix() const
	{
		// initialise directly
		fVec3 const& col1 = m_basis[0];
		fVec3 const& col2 = m_basis[1];
		fVec3 const& col3 = m_basis[2];

		fVec3 const& posi = m_origin;

		return {
			col1[0], col1[1], col1[2], 0.0f,
			col2[0], col2[1], col2[2], 0.0f,
			col3[0], col3[1], col3[2], 0.0f,
			posi[0], posi[1], posi[2], 1.0f,
		};
	}

	btTransform GetBulletTransform() const
	{
		// initialise directly
		return btTransform(
			btMatrix3x3(
				m_basis[0][0], m_basis[1][0], m_basis[2][0],
				m_basis[0][1], m_basis[1][1], m_basis[2][1],
				m_basis[0][2], m_basis[1][2], m_basis[2][2]),
			btVector3(m_origin[0], m_origin[1], m_origin[2])
		);
	}

	fTrans ToLocal(fTrans const& _a) const
	{
		fMat3 const invB = glm::inverse(m_basis);
		return fTrans(_a.m_basis * invB, (_a.m_origin - m_origin) * invB);
	}

	fTrans() = default;
	fTrans(fMat3 const& _b, fVec3 const& _o = fVec3(0.0f))
		: m_basis(_b)
		, m_origin(_o)
	{}
	fTrans(fQuat const& _q, fVec3 const& _o = fVec3(0.0f))
		: m_basis(_q)
		, m_origin(_o)
	{}

	explicit fTrans(btTransform const& _t)
		: m_basis{
		_t.getBasis().getRow(0).x(), _t.getBasis().getRow(1).x(), _t.getBasis().getRow(2).x(),
		_t.getBasis().getRow(0).y(), _t.getBasis().getRow(1).y(), _t.getBasis().getRow(2).y(),
		_t.getBasis().getRow(0).z(), _t.getBasis().getRow(1).z(), _t.getBasis().getRow(2).z(),
	}
		, m_origin{
		_t.getOrigin().x(), _t.getOrigin().y(), _t.getOrigin().z()
	}
	{}
};

inline fMat3 RotationFromForward(fVec3 const& _f)
{
	fVec3 const normF = glm::normalize(_f);
	fVec3 const right = glm::normalize(glm::cross(normF, fVec3(0.0f, 1.0f, 0.0f)));
	fVec3 const up = glm::normalize(glm::cross(right, normF));
	return fMat3{
		right.x,	right.y,	right.z,	// x-right
		up.x,		up.y,		up.z,		// y-up
		normF.x,	normF.y,	normF.z,	// z-forward
	};
}

inline fVec3 ConvertFrombtVector3(btVector3 const& _btVec3)
{
	return fVec3(_btVec3.x(), _btVec3.y(), _btVec3.z());
}
inline btVector3 ConvertTobtVector3(fVec3 const& _fVec3)
{
	return btVector3(_fVec3.x, _fVec3.y, _fVec3.z);
}

namespace Colour
{
	constexpr uint32 white = static_cast<uint32>(-1);
	constexpr uint32 green = static_cast<uint32>(65280u);

	inline constexpr uint32 ConvertRGB(fVec3 const& _col)
	{
		uint32 const r = static_cast<uint32>(gcem::round(_col.r * 255.0f));
		uint32 const g = static_cast<uint32>(gcem::round(_col.g * 255.0f));
		uint32 const b = static_cast<uint32>(gcem::round(_col.b * 255.0f));
		constexpr uint32 const a = 0xFF;
		return (a << 24) + (b << 16) + (g << 8) + (r);
	}
}

#if DEBUG_TOOLS
#include <assert.h>

#if _MSC_VER
#define ASSERT_MESSAGE(cond, message) (void)(                                            \
            (!!(cond)) ||                                                                \
            (_wassert(_CRT_WIDE(message), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \
        )
#else
#define ASSERT_MESSAGE(cond, message) assert(cond)
#endif

#define GET_ASSERT_MACRO(_1,_2,NAME,...) NAME
#define ASSERT(...) GET_ASSERT_MACRO(__VA_ARGS__, ASSERT_MESSAGE, assert)(__VA_ARGS__)

#else

#define ASSERT(...)

#endif

#define SafeDelete(OBJ) { delete OBJ; OBJ = nullptr; }