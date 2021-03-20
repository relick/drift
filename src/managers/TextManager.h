#pragma once

#include "common.h"

#include "ResourceIDs.h"

#define TEXT_TEST DEBUG_TOOLS

namespace Core::Render::Text
{
	void Init();
	void Setup();
	void Render();
	void Cleanup();
	
#if TEXT_TEST
	void ShowDebugText();
#endif

	bool Write(Resource::FontID _font, fVec2 _tlPos, char const* _text, float _size = 16.0f, uint32 _col = Colour::green);
	bool Write(fVec2 _tlPos, char const* _text, float _size = 16.0f, uint32 _col = Colour::white);
}