#include "EntityManager.h"

namespace Core
{
	EntityID::CoreType nextID = 0;

	EntityID CreateEntity()
	{
		return EntityID(nextID++);
	}
}