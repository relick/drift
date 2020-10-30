#include "EntityManager.h"

namespace Core
{
	ecs::entity_id nextID = 0;

	ecs::entity_id CreateEntity()
	{
		return nextID++;
	}
}