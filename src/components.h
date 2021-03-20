#pragma once

#include "common.h"

#include <ecs/ecs.h>

// Core
namespace Core
{
	// Empty global component, take by ref in any system group that needs to run without parallelisation.
	struct MT_Only
	{
		ecs_flags(ecs::flag::global);
	};
}

// Required for Add/RemoveComponent overloads
#include "managers/EntityManager.h"

#include "components/Core/CameraComponents.h"
#include "components/Core/FrameComponents.h"
#include "components/Core/PhysicsComponents.h"
#include "components/Core/RenderComponents.h"
#include "components/Core/ResourceComponents.h"
#include "components/Core/SoundComponents.h"
#include "components/Core/TransformComponents.h"

// Game
#include "components/Game/PlayerComponents.h"
#include "components/Game/UIComponents.h"