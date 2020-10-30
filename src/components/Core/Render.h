#pragma once

#include <ecs/component_specifier.h>

namespace Core
{
	namespace Render
	{
		struct FrameData
		{
			ecs_flags(ecs::flag::global);

			int w{ 0 };
			int h{ 0 };
			float fW{ 0.0f };
			float fH{ 0.0f };
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