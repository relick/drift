#pragma once

#include "common.h"

#include "ResourceIDs.h"

#if DEBUG_TOOLS
	#define TEXT_TEST 1
#endif

struct sapp_event;

namespace Core::Render::Text
{
	void Init();
	void Setup();
	void Render();
	void Cleanup();
	void Event(sapp_event const* _event);
	
#if TEXT_TEST
	void ShowDebugText();
#endif

	bool Write(Resource::FontID _font, Vec2 _tlPos, char const* _text, Vec1 _size = 16.0f, uint32 _col = Colour::green);
	bool Write(Vec2 _tlPos, char const* _text, Vec1 _size = 16.0f, uint32 _col = Colour::white);
}