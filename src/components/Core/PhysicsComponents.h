#pragma once

#include "common.h"
#include "managers/EntityManager.h"

#include <ecs/flags.h>

// forward
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionShape;
struct btDefaultMotionState;
class btRigidBody;

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
			float m_mass{ 0 };
			ShapeType m_shapeType{ ShapeType::Box };
			fTrans m_startTransform{};

			// Box
			fVec3 m_boxHalfDimensions{};

			// Sphere
			float m_radius{};
		};

		struct RigidBody
		{
			use_initialiser;

			EntityID m_physicsWorld{};

			btCollisionShape* m_shape{ nullptr };
			btDefaultMotionState* m_motionState{ nullptr };
			btRigidBody* m_body{ nullptr };
		};

		struct CharacterControllerDesc
		{
			EntityID m_physicsWorld{};
			EntityID m_viewObject{}; // local transform used for forward direction.

			float m_mass{ 80 }; //kg
			fTrans m_startTransform{};
			float m_radius{ 0.5f };
			float m_halfHeight{ 1.0f };
		};

		struct CharacterController : public RigidBody
		{
			EntityID m_viewObject{};
			float m_radius{ 0.5f };
			float m_halfHeight{ 1.0f };
		};

		Core::EntityID GetPrimaryWorldEntity();

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

	template<>
	void AddComponent(EntityID const _entity, Physics::CharacterControllerDesc const& _desc);

	template<>
	void RemoveComponent<Physics::CharacterController>(EntityID const _entity);

}