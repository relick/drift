#pragma once

#define IMGUI_DEBUG_ENABLED DEBUG_TOOLS

// Define some number types
#include <cstddef>
#include <cstdint>
#include <cstring>
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

	fMat4 GetRenderMatrix() const
	{
		// initialise directly
		fVec3 const& col1 = m_basis[0];
		fVec3 const& col2 = m_basis[1];
		fVec3 const& col3 = m_basis[2];

		fVec3 const& row4 = m_origin;

		return {
			col1[0], col2[0], col3[0], 0.0f,
			col1[1], col2[1], col3[1], 0.0f,
			col1[2], col2[2], col3[2], 0.0f,
			row4[0], row4[1], row4[2], 1.0f,
		};
	}

	btTransform GetBulletTransform() const
	{
		// initialise directly
		return btTransform(
			btMatrix3x3(
				m_basis[0][0], m_basis[0][1], m_basis[0][2], 
				m_basis[1][0], m_basis[1][1], m_basis[1][2], 
				m_basis[2][0], m_basis[2][1], m_basis[2][2]),
			btVector3(m_origin[0], m_origin[1], m_origin[2])
		);
	}

	fTrans() = default;
	fTrans(fMat3 const& _b, fVec3 const& _o = fVec3(0.0f))
		: m_basis(_b)
		, m_origin(_o)
	{}

	explicit fTrans(btTransform const& _t)
		: m_basis{
		_t.getBasis().getRow(0).x(), _t.getBasis().getRow(0).y(), _t.getBasis().getRow(0).z(),
		_t.getBasis().getRow(1).x(), _t.getBasis().getRow(1).y(), _t.getBasis().getRow(1).z(),
		_t.getBasis().getRow(2).x(), _t.getBasis().getRow(2).y(), _t.getBasis().getRow(2).z(),
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

#define WINDOW_START_WIDTH 640
#define WINDOW_START_HEIGHT 480

#define COL_WHITE static_cast<uint32>(-1)

#if DEBUG_TOOLS
#include <assert.h>
// Nothing special yet but maybe one day
#define ASSERT(TEST) assert( TEST ) 
#else
#define ASSERT(TEST)
#endif

#define SafeDelete(OBJ) delete OBJ; OBJ = nullptr;