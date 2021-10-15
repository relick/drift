#pragma once

#include "common.h"
#include "MT_Only.h"

#include <ecs/ecs.h>

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