#pragma once

#include "common.h"

class btIDebugDraw;

namespace Core
{
	namespace Physics
	{
		void Init();
		void Setup();
		void Cleanup();

#if DEBUG_TOOLS
		btIDebugDraw* GetDebugDrawer();
#endif
	}
}