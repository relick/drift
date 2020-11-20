#pragma once

#include "common.h"

#include <ecs/flags.h>

namespace Core
{
	struct FrameData
	{
		ecs_flags(ecs::flag::global, ecs::flag::immutable);

		uint64 m_lastFrameTicks{ 0 };
		double m_scale{ 0.0 };
		double unscaled_ddt{ 0.0 };
		float unscaled_dt{ 0.0f };
		double ddt{ 0.0 };
		float dt{ 0.0f };
	};
}