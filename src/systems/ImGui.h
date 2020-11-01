#pragma once

#include "common.h"

#include <ecs/entity_id.h>

struct sapp_event;

namespace Core
{
	namespace Render
	{
		namespace DImGui
		{
			void Init();
			void Setup(ecs::entity_id _renderEnt);
			void Render();
			void Cleanup();
			void Event(sapp_event const* _event);

			void AddMenuItem(char const* _header, char const* _name, bool* _open);
		}
	}
}