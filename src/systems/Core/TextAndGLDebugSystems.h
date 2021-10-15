#pragma once

#include "common.h"

struct sapp_event;

namespace Core
{
	namespace Render
	{
		namespace TextAndGLDebug
		{
			void Init();
			void Setup();
			void Render();
			void Cleanup();
			void Event(sapp_event const* _event);
		}

		namespace Debug
		{
			void DrawLine(Vec3 const& _start, Vec3 const& _end, uint32 _col = Colour::white);
			void DrawLine(Vec3 const& _start, Vec3 const& _end, Vec3 const& _col);
		}
	}
}