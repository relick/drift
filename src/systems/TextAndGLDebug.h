#pragma once

#include "common.h"

namespace Core
{
	namespace Render
	{
		void TextAndGLDebugSetup();
		void TextAndGLDebugRender(int _w, int _h);
		void TextAndGLDebugCleanup();
	}
}