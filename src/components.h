#pragma once

#include "common.h"

#include <ecs/ecs.h>

// Core
namespace Core
{
	// Only one entity will get this, allows for systems with only global components to run once per frame
	struct GlobalWorkaround_Tag
	{
		ecs_flags(ecs::flag::tag);
	};

	// Empty global component, take by ref in any system group that needs to run without parallelisation.
	struct MT_Only
	{
		ecs_flags(ecs::flag::global);
	};
}

// Required for Add/RemoveComponent overloads
#include "managers/EntityManager.h"

#include "components/Core/Camera.h"
#include "components/Core/Frame.h"
#include "components/Core/Physics.h"
#include "components/Core/Render.h"
#include "components/Core/Transform.h"

// Game
