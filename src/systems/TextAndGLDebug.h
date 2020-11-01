#pragma once

#include "common.h"

namespace Core
{
	namespace Render
	{
		namespace TextAndGLDebug
		{
			void Init();
			void Render(int _w, int _h);
			void Cleanup();

			bool RenderText(int32 _fontI, fVec2 _tlPos, char const* _text, float _size = 16.0f, uint32 _col = COL_WHITE);
		}
	}
}