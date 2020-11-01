#pragma once

#include "common.h"

struct sapp_event;

namespace Core
{
	namespace Render
	{
		namespace DImGui
		{
			void Init();
			void Setup();
			void Render();
			void Cleanup();
			void Event(sapp_event const* _event);

			void AddMenuItem(char const* _header, char const* _name, bool* _open);
		}
	}
}