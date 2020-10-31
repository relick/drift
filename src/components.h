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
}

#include "components/Core/Frame.h"
#include "components/Core/Render.h"
#include "components/Core/Transform.h"

// Game