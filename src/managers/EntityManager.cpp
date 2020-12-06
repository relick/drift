#include "EntityManager.h"

namespace Core
{
	ecs::entity_id nextID = 0;

	EntityID CreateEntity()
	{
		return EntityID(nextID++);
	}
}