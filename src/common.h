#pragma once

// relatively global defines
#if DEBUG_TOOLS
	#define IMGUI_DEBUG_ENABLED 1
#endif

// commmon headers
#include "common/MathDefs.h"
#include "common/Debug.h"
#include "common/fTrans.h"
#include "common/Colour.h"

// little extras that don't need a separate header
template<typename T>
constexpr void SafeDelete(T*& _obj)
{
	delete _obj;
	_obj = nullptr;
}

#define SG_RANGE_VEC(x) sg_range{ x.data(), sizeof(decltype(x)::value_type) * x.size() }

// component helpers
#define use_initialiser struct _initialiser_only {}
#define not_a_component struct _not_a_component {}