#pragma once

#ifndef _DEBUG
#define DEBUG 0
#endif

#define IMGUI_DEBUG_ENABLED _DEBUG
#define SOKOL_GLCORE33

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
#include <Bullet3Common/b3Vector3.h>
#include <Bullet3Common/b3Matrix3x3.h>
#include <Bullet3Common/b3Transform.h>
#include <Bullet3Common/b3Quaternion.h>

// f == float
using fVec3 = b3Vector3;
using fVec3Data = b3Vector3Data;
#define LoadVec3 b3MakeVector3
using fVec4 = b3Vector4;
using fVec4Data = b3Vector3Data;
#define LoadVec4 b3MakeVector4
using fQuat = b3Quaternion;
using fTrans = b3Transform;
using fTransData = b3TransformData;
using fMat3 = b3Matrix3x3;
using fMat3Data = b3Matrix3x3Data;

#define WINDOW_START_WIDTH 640
#define WINDOW_START_HEIGHT 480