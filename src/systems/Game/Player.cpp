#include "Player.h"

#include "components.h"
#include "systems/Core/SystemOrdering.h"
#include <ecs/ecs.h>

namespace Game
{
	void Setup()
	{
		ecs::make_system<ecs::opts::group<Sys::GAME>>([](Player& _p)
		{

		});
	}
}