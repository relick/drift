#pragma once

#define IMGUI_DEBUG_ENABLED DEBUG_TOOLS

// Define some number types
#include <cstddef>
#include <cstdint>
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
union fVec2
{
	float m_floats[2];
	struct
	{
		float x;
		float y;
	};

	fVec2(float _x, float _y) : x(_x), y(_y) {}
};
using fVec3 = btVector3;
using fVec3Data = btVector3Data;
using fVec4 = btVector4;
using fVec4Data = btVector3Data;
using fQuat = btQuaternion;
using fTrans = btTransform;
using fTransData = btTransformData;
using fMat3 = btMatrix3x3;
using fMat3Data = btMatrix3x3Data;


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