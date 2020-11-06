#pragma once

#include "common.h"
#include "managers/EntityManager.h"
#include <ecs/component_specifier.h>

#include <btBulletDynamicsCommon.h>


namespace Core
{
	namespace Physics
	{
		struct World
		{
			btDefaultCollisionConfiguration* m_collisionConfiguration{ nullptr };
			btCollisionDispatcher* m_dispatcher{ nullptr };
			btBroadphaseInterface* m_overlappingPairCache{ nullptr };
			btSequentialImpulseConstraintSolver* m_solver{ nullptr };
			btDiscreteDynamicsWorld* m_dynamicsWorld{ nullptr };
		};

		enum class ShapeType
		{
			Box,
			Sphere,
		};

		struct RigidBodyDesc
		{
			EntityID m_physicsWorld{};

			bool m_isKinematic{ false };
			btScalar m_mass{ 0 };
			ShapeType m_shapeType{ ShapeType::Box };
			btTransform m_startTransform{};

			// Box
			btVector3 m_boxDimensions{};

			// Sphere
			btScalar m_radius{};
		};

		struct RigidBody
		{
			use_initialiser;

			EntityID m_physicsWorld{};

			btCollisionShape* m_shape{ nullptr };
			btDefaultMotionState* m_motionState{ nullptr };
			btRigidBody* m_body{ nullptr };
		};

		inline Physics::World& GetWorld(EntityID const _physicsWorld)
		{
			Core::Physics::World* const physicsWorld = ecs::get_component<Core::Physics::World>(_physicsWorld.GetValue());
			ASSERT(physicsWorld);
			return *physicsWorld;
		}
	}

	template<>
	void AddComponent(EntityID const _entity, Physics::World const& _component);

	template<>
	void RemoveComponent<Physics::World>(EntityID const _entity);

	template<>
	void AddComponent(EntityID const _entity, Physics::RigidBodyDesc const& _desc);

	template<>
	void RemoveComponent<Physics::RigidBody>(EntityID const _entity);

}