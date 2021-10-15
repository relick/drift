#pragma once

#include "common.h"

#include <ecs/flags.h>

namespace Core
{
	struct FrameData
	{
		ecs_flags(ecs::flag::global, ecs::flag::immutable);

		uint64 m_lastFrameTicks{ 0 };
		dVec1 m_scale{ 0.0 };
		dVec1 m_unpausedSpeed{ 1.0 };
		dVec1 unscaled_ddt{ 0.0 };
		Vec1 unscaled_dt{ 0.0f };
		dVec1 ddt{ 0.0 };
		Vec1 dt{ 0.0f };

#if DEBUG_TOOLS
		dVec1 m_debug_elapsedTime{ 0.0 };
		uint64 m_debug_frameCount{ 0 };
#endif
	};
}