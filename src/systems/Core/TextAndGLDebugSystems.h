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
			void DrawLine(fVec3 const& _start, fVec3 const& _end, uint32 _col = Colour::white);
			void DrawLine(fVec3 const& _start, fVec3 const& _end, fVec3 const& _col);
		}
	}
}