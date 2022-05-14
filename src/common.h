#pragma once

// relatively global defines
#if DEBUG_TOOLS
	#define IMGUI_DEBUG_ENABLED 1
#endif

// commmon headers
#include "common/MathDefs.h"
#include "common/Debug.h"
#include "common/Transforms.h"
#include "common/Colour.h"
#include "common/Bit.h"
#include "common/Rect.h"
//#include "common/Mutex.h" // This can be included manually where needed.

// little extras that don't need a separate header
template<typename T>
constexpr void SafeDelete(T*& _obj)
{
	if (_obj != nullptr)
	{
		delete _obj;
	}
	_obj = nullptr;
}

#define SG_RANGE_VEC(x) sg_range{ x.data(), sizeof(decltype(x)::value_type) * x.size() }

// component helpers
#define use_initialiser struct _initialiser_only {}
#define not_a_component struct _not_a_component {}

// GLSL header cross-usage
#define GLSL_CONSTANT inline constexpr
#define GLSL_UINT32 uint32
