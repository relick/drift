#pragma once

#include "common.h"

#include <ecs/component_specifier.h>

namespace Core
{
	namespace Render
	{
		// Attach to the entity with the pass tag you want
		struct Camera
		{
			fVec2 angle{ 0.0f, 0.0f };
			float m_povY{ 60.0f }; // degrees
		};

		struct DebugCameraControl_Tag
		{
			ecs_flags(ecs::flag::tag);
		};
	}
}