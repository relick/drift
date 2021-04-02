#include "PhysicsComponents.h"

#include "systems/Core/PhysicsSystems.h"

#include <btBulletDynamicsCommon.h>

#include <map>
struct PhysicsWorldInternalData
{
	uint32 numBodies{ 0u };
};
static std::map<Core::EntityID, PhysicsWorldInternalData> g_physicsWorlds;

namespace Core
{
	namespace Physics
	{
		Core::EntityID GetPrimaryWorldEntity()
		{
			kaAssert(!g_physicsWorlds.empty());
			return g_physicsWorlds.begin()->first;
		}
	}
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
			kaAssert(newComponent.m_dispatcher);
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

		kaAssert(!newComponent.m_dynamicsWorld);
		newComponent.m_dynamicsWorld = new btDiscreteDynamicsWorld(newComponent.m_dispatcher, newComponent.m_overlappingPairCache, newComponent.m_solver, newComponent.m_collisionConfiguration);
		newComponent.m_dynamicsWorld->setGravity(btVector3(0, -8.0f, 0));

#if PHYSICS_DEBUG
		newComponent.m_dynamicsWorld->setDebugDrawer(Physics::Debug::GetDebugDrawer());
		Physics::Debug::AddPhysicsWorld(_entity);
#endif
		g_physicsWorlds.emplace(_entity, PhysicsWorldInternalData{ 0u });

		Core::ECS::AddComponent(_entity, newComponent);
	}

	template<>
	void CleanupComponent<Physics::World>(EntityID const _entity)
	{
		Physics::World* const oldComponent = Core::GetComponent<Physics::World>(_entity);
		kaAssert(oldComponent);
		SafeDelete(oldComponent->m_dynamicsWorld);
		SafeDelete(oldComponent->m_solver);
		SafeDelete(oldComponent->m_overlappingPairCache);
		SafeDelete(oldComponent->m_dispatcher);
		SafeDelete(oldComponent->m_collisionConfiguration);

#if PHYSICS_DEBUG
		Physics::Debug::RemovePhysicsWorld(_entity);
#endif
		kaAssert(g_physicsWorlds.at(_entity).numBodies == 0u);
		g_physicsWorlds.erase(_entity);
	}

	template<>
	void AddComponent(EntityID const _entity, Physics::RigidBodyDesc const& _desc)
	{
		Physics::RigidBody newComponent{};

		kaAssert(_desc.m_physicsWorld.IsValid());
		newComponent.m_physicsWorld = _desc.m_physicsWorld;

		// Select shape
		switch (_desc.m_shapeType)
		{
		case Physics::ShapeType::Box:
		{
			newComponent.m_shape = new btBoxShape(ConvertTobtVector3(_desc.m_boxHalfDimensions));
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

		newComponent.m_motionState = new btDefaultMotionState(_desc.m_startTransform.GetBulletTransform());

		// Finalise RB
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, newComponent.m_motionState, newComponent.m_shape, localInertia);
		newComponent.m_body = new btRigidBody(rbInfo);

		if (_desc.m_isKinematic)
		{
			int const existingFlags = newComponent.m_body->getCollisionFlags();
			newComponent.m_body->setCollisionFlags(existingFlags | btCollisionObject::CF_KINEMATIC_OBJECT);
		}

		// Add to physics world
		Core::Physics::World& physicsWorld = Physics::GetWorld(newComponent.m_physicsWorld);
		physicsWorld.m_dynamicsWorld->addRigidBody(newComponent.m_body);

		g_physicsWorlds.at(newComponent.m_physicsWorld).numBodies++;

		// Add to ecs
		Core::ECS::AddComponent(_entity, newComponent);
	}

	template<>
	void CleanupComponent<Physics::RigidBody>(EntityID const _entity)
	{
		Physics::RigidBody* const oldComponent = Core::GetComponent<Physics::RigidBody>(_entity);
		kaAssert(oldComponent);

		Core::Physics::World& physicsWorld = Physics::GetWorld(oldComponent->m_physicsWorld);
		physicsWorld.m_dynamicsWorld->removeCollisionObject(oldComponent->m_body);

		g_physicsWorlds.at(oldComponent->m_physicsWorld).numBodies--;

		SafeDelete(oldComponent->m_body);
		SafeDelete(oldComponent->m_motionState);
		SafeDelete(oldComponent->m_shape);
	}

	template<>
	void AddComponent(EntityID const _entity, Physics::CharacterControllerDesc const& _desc)
	{
		Physics::CharacterController newComponent{};

		kaAssert(_desc.m_physicsWorld.IsValid());
		newComponent.m_physicsWorld = _desc.m_physicsWorld;
		newComponent.m_viewObject = _desc.m_viewObject;

		newComponent.m_radius = _desc.m_radius;
		newComponent.m_halfHeight = _desc.m_halfHeight;
		newComponent.m_shape = new btCapsuleShape(_desc.m_radius, _desc.m_halfHeight * 2.0f);

		btScalar const mass = std::max(_desc.m_mass, 0.0f);
		btVector3 localInertia(0, 0, 0);
		newComponent.m_shape->calculateLocalInertia(mass, localInertia);

		newComponent.m_motionState = new btDefaultMotionState(_desc.m_startTransform.GetBulletTransform());

		// Finalise RB
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, newComponent.m_motionState, newComponent.m_shape, localInertia);
		newComponent.m_body = new btRigidBody(rbInfo);

		newComponent.m_body->setSleepingThresholds(0.0f, 0.0f);
		newComponent.m_body->setAngularFactor(0.0f);

		// Add to physics world
		Core::Physics::World& physicsWorld = Physics::GetWorld(newComponent.m_physicsWorld);
		physicsWorld.m_dynamicsWorld->addRigidBody(newComponent.m_body);

		g_physicsWorlds.at(newComponent.m_physicsWorld).numBodies++;

		// Add to ecs
		Core::ECS::AddComponent(_entity, newComponent);
	}

	template<>
	void CleanupComponent<Physics::CharacterController>(EntityID const _entity)
	{
		Physics::CharacterController* const oldComponent = Core::GetComponent<Physics::CharacterController>(_entity);
		kaAssert(oldComponent);

		Core::Physics::World& physicsWorld = Physics::GetWorld(oldComponent->m_physicsWorld);
		physicsWorld.m_dynamicsWorld->removeCollisionObject(oldComponent->m_body);

		g_physicsWorlds.at(oldComponent->m_physicsWorld).numBodies--;

		SafeDelete(oldComponent->m_body);
		SafeDelete(oldComponent->m_motionState);
		SafeDelete(oldComponent->m_shape);
	}
}