#pragma once

#include <ecs/ecs.h>

namespace Core
{
	extern ecs::entity_id nextID;

	ecs::entity_id CreateEntity();
}