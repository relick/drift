#pragma once

#include "Entity.h"
#include "common.h"

class btIDebugDraw;
namespace physx
{
	class PxPhysics;
}
#define PHYSICS_DEBUG DEBUG_TOOLS

namespace Core
{
	namespace Physics
	{
		void Init();
		void Setup();
		void Cleanup();

#if PHYSICS_DEBUG
		btIDebugDraw* GetDebugDrawer();
		void AddPhysicsWorld(Core::EntityID _entity);
		void RemovePhysicsWorld(Core::EntityID _entity);
#endif
	}
}