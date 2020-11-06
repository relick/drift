#include "Physics.h"

namespace Core
{
	template<>
	void AddComponent(EntityID const _entity, Physics::World const& _component)
	{
		Physics::World newComponent = _component;
		if (!newComponent.m_collisionConfiguration)
		{
			newComponent.m_collisionConfiguration = new btDefaultCollisionConfiguration();
		}
		else
		{
			ASSERT(newComponent.m_dispatcher);
		}
		if (!newComponent.m_dispatcher)
		{
			newComponent.m_dispatcher = new btCollisionDispatcher(newComponent.m_collisionConfiguration);
		}
		if (!newComponent.m_overlappingPairCache)
		{
			newComponent.m_overlappingPairCache = new btDbvtBroadphase();
		}
		if (!newComponent.m_solver)
		{
			newComponent.m_solver = new btSequentialImpulseConstraintSolver();
		}

		ASSERT(!newComponent.m_dynamicsWorld);
		newComponent.m_dynamicsWorld = new btDiscreteDynamicsWorld(newComponent.m_dispatcher, newComponent.m_overlappingPairCache, newComponent.m_solver, newComponent.m_collisionConfiguration);
		newComponent.m_dynamicsWorld->setGravity(btVector3(0, -8.0f, 0));

		ecs::add_component(_entity.GetValue(), newComponent);
	}

	template<>
	void RemoveComponent<Physics::World>(EntityID const _entity)
	{
		Physics::World* const oldComponent = ecs::get_component<Physics::World>(_entity.GetValue());
		ASSERT(oldComponent);
		SafeDelete(oldComponent->m_collisionConfiguration);
		SafeDelete(oldComponent->m_dispatcher);
		SafeDelete(oldComponent->m_overlappingPairCache);
		SafeDelete(oldComponent->m_solver);
		SafeDelete(oldComponent->m_dynamicsWorld);

		ecs::remove_component<Physics::World>(_entity.GetValue());
	}

	template<>
	void AddComponent(EntityID const _entity, Physics::RigidBodyDesc const& _desc)
	{
		Physics::RigidBody newComponent{};

		ASSERT(_desc.m_physicsWorld.IsValid());
		newComponent.m_physicsWorld = _desc.m_physicsWorld;

		// Select shape
		switch (_desc.m_shapeType)
		{
		case Physics::ShapeType::Box:
		{
			newComponent.m_shape = new btBoxShape(_desc.m_boxDimensions);
			break;
		}
		case Physics::ShapeType::Sphere:
		{
			newComponent.m_shape = new btSphereShape(_desc.m_radius);
			break;
		}
		}

		btScalar const mass = std::max(_desc.m_mass, 0.0f);
		bool const isDynamic = _desc.m_mass > 0.0f;

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
		{
			newComponent.m_shape->calculateLocalInertia(mass, localInertia);
		}

		newComponent.m_motionState = new btDefaultMotionState(_desc.m_startTransform);

		// Finalise RB
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, newComponent.m_motionState, newComponent.m_shape, localInertia);
		newComponent.m_body = new btRigidBody(rbInfo);

		// Add to physics world
		Core::Physics::World& physicsWorld = Physics::GetWorld(newComponent.m_physicsWorld);
		physicsWorld.m_dynamicsWorld->addRigidBody(newComponent.m_body);

		// Add to ecs
		ecs::add_component(_entity.GetValue(), newComponent);
	}

	template<>
	void RemoveComponent<Physics::RigidBody>(EntityID const _entity)
	{
		Physics::RigidBody* const oldComponent = ecs::get_component<Physics::RigidBody>(_entity.GetValue());
		ASSERT(oldComponent);

		Core::Physics::World& physicsWorld = Physics::GetWorld(oldComponent->m_physicsWorld);
		physicsWorld.m_dynamicsWorld->removeCollisionObject(oldComponent->m_body);

		SafeDelete(oldComponent->m_motionState);
		SafeDelete(oldComponent->m_body);
		SafeDelete(oldComponent->m_shape);

		ecs::remove_component<Physics::World>(_entity.GetValue());
	}
}