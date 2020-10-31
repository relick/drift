#pragma once

#include <ecs/component_specifier.h>

namespace Core
{
	namespace Render
	{
		struct FrameData
		{
			ecs_flags(ecs::flag::global);

			int w{ WINDOW_START_WIDTH };
			int h{ WINDOW_START_HEIGHT };
			float fW{ static_cast<float>(w) };
			float fH{ static_cast<float>(h) };
		};
		struct Frame_Tag
		{
			ecs_flags(ecs::flag::tag);
		};
		struct DefaultPass_Tag
		{
			ecs_flags(ecs::flag::tag);
		};
	}
}