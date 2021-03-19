#pragma once

// relatively global defines
#if DEBUG_TOOLS
	#define IMGUI_DEBUG_ENABLED 1
#endif

// little extras that don't need a separate header
template<typename T>
constexpr void SafeDelete(T*& _obj)
{
	delete _obj;
	_obj = nullptr;
}

#define SG_RANGE_VEC(x) sg_range{ x.data(), sizeof(decltype(x)::value_type) * x.size() }

// commmon headers
#include "common/MathDefs.h"
#include "common/fTrans.h"
#include "common/Colour.h"
#include "common/Debug.h"