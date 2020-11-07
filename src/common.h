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
#include <LinearMath/btVector3.h>
#include <LinearMath/btMatrix3x3.h>
#include <LinearMath/btTransform.h>
#include <LinearMath/btQuaternion.h>

// f == float
union fVec2Data
{
	float m_floats[2];
	struct
	{
		float x;
		float y;
	};

	fVec2Data() : x(0.0f), y(0.0f) {}
	fVec2Data(float _x, float _y) : x(_x), y(_y) {}
};

union fVec3Data
{
	float m_floats[3];
	struct
	{
		float x;
		float y;
		float z;
	};

	fVec3Data() : x(0.0f), y(0.0f), z(0.0f) {}
	fVec3Data(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

union fVec4Data
{
	float m_floats[4];
	struct
	{
		float x;
		float y;
		float z;
		float w;
	};

	fVec4Data() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
	fVec4Data(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

using fVec2 = fVec2Data; // no sse here
using fVec3 = btVector3;
using fVec4 = btVector4;
using fQuat = btQuaternion;
using fTrans = btTransform;
using fTransData = btTransformData;
using fMat3 = btMatrix3x3;
using fMat3Data = btMatrix3x3Data;

#include "HandmadeMath.h"

HINLINE hmm_mat4
HMM_Mat4FromfTrans(fTrans const& _trans)
{
	// initialise directly
	fVec3 const& col1 = _trans.getBasis().getRow(0);
	fVec3 const& col2 = _trans.getBasis().getRow(1);
	fVec3 const& col3 = _trans.getBasis().getRow(2);

	fVec3 const& row4 = _trans.getOrigin();

	hmm_mat4 Result{
		col1.m_floats[0], col2.m_floats[0], col3.m_floats[0], 0.0f,
		col1.m_floats[1], col2.m_floats[1], col3.m_floats[1], 0.0f,
		col1.m_floats[2], col2.m_floats[2], col3.m_floats[2], 0.0f,
		row4.m_floats[0], row4.m_floats[1], row4.m_floats[2], 1.0f,
	};


	/*// initialise then fill
	hmm_mat4 Result = HMM_Mat4d(1.0f);
	// copy in rotation
	std::memcpy(&Result.Elements[0][0], &_trans.getBasis().getRow(0).m_floats[0], sizeof(float) * 12);
	//_trans.getBasis().serializeFloat(*reinterpret_cast<btMatrix3x3FloatData*>(Result.Elements));
	Result = HMM_Transpose(Result);
	// copy in translation
	std::memcpy(&Result.Elements[3][0], &_trans.getOrigin().m_floats[0], sizeof(float) * 3);
	//_trans.getOrigin().serializeFloat(*reinterpret_cast<btVector3FloatData*>(&Result.Elements[3]));
	// fill in final item that we haven't written.
	Result.Elements[3][3] = 1.0f;*/

	return (Result);
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