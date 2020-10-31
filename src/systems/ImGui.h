#pragma once

#include "common.h"

struct sapp_event;

namespace Core
{
	namespace Render
	{
		void ImGuiSetup();
		void ImGuiRender();
		void ImGuiCleanup();
		void ImGuiEvent(sapp_event const* _event);
	}
}