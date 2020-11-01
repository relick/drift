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

		namespace DImGui
		{
			// ImGui systems need to not run in parallel, so they should all run on an entity with this component, taking it as a non-const ref
			// Some data could be stored here if necessary, though I doubt it
			struct ImGuiData
			{

			};
		}
	}
}