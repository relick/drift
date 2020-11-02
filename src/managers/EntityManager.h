#pragma once

#include <ecs/ecs.h>
#include "Entity.h"

namespace Core
{
	extern ecs::entity_id nextID;

	EntityID CreateEntity();
}