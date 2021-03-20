#pragma once

#include "common.h"

#include "components/Core/TransformComponents.h"

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
		}

		namespace Debug
		{
			void DrawLine(fVec3 const& _start, fVec3 const& _end, uint32 _col = Colour::white);
			void DrawLine(fVec3 const& _start, fVec3 const& _end, fVec3 const& _col);
		}

		namespace Text
		{
			bool Write(uint32 _fontI, fVec2 _tlPos, char const* _text, float _size = 16.0f, uint32 _col = Colour::green);
			bool Write(fVec2 _tlPos, char const* _text, float _size = 16.0f, uint32 _col = Colour::white);
		}
	}
}