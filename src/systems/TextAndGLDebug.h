#pragma once

#include "common.h"

#include "components/Core/Transform.h"

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

			bool RenderText(uint32 _fontI, fVec2 _tlPos, char const* _text, float _size = 16.0f, uint32 _col = COL_WHITE);
		}
	}
}