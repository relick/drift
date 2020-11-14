#pragma once

#include "common.h"
#include "managers/EntityManager.h"
#include <ecs/component_specifier.h>

#include "PxPhysicsAPI.h"

namespace Core
{
	namespace Physics
	{
		struct World
		{
			physx::PxCpuDispatcher* m_dispatcher{ nullptr };
			physx::PxScene* m_scene{ nullptr };
			physx::PxMaterial* m_defaultMat{ nullptr };
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
			fTrans m_startTransform{};

			// Box
			physx::PxVec3 m_boxHalfDimensions{};

			// Sphere
			float m_radius{};
		};

		struct RigidBody
		{
			use_initialiser;

			EntityID m_physicsWorld{};

			physx::PxGeometry* m_geom{ nullptr };
			bool m_isStatic{ nullptr };
			physx::PxRigidActor* m_body{ nullptr };
		};

		struct CharacterControllerDesc
		{
			EntityID m_physicsWorld{};

			btScalar m_mass{ 80 }; //kg
			fTrans m_startTransform{};
			btScalar m_radius{ 0.5f };
			btScalar m_halfHeight{ 1.0f };
		};

		struct CharacterController : public RigidBody
		{
			btScalar m_halfHeight{ 1.0f };
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