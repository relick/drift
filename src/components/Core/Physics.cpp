#include "Physics.h"

#include "systems/Core/Physics.h"

#include "PxPhysicsAPI.h"

#include <map>
struct PhysicsWorldInternalData
{
	uint32 numBodies{ 0u };
};
std::map<Core::EntityID, PhysicsWorldInternalData> physicsWorlds;

namespace Core
{
	namespace Physics
	{
		Core::EntityID GetPrimaryWorldEntity()
		{
			ASSERT(!physicsWorlds.empty());
			return physicsWorlds.begin()->first;
		}
	}

	template<>
	void AddComponent(EntityID const _entity, Physics::World const& _component)
	{
		Physics::World newComponent = _component;
		physx::PxSceneDesc sceneDesc(PxGetPhysics().getTolerancesScale());
		sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
		newComponent.m_dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
		sceneDesc.cpuDispatcher = newComponent.m_dispatcher;
		sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
		newComponent.m_scene = PxGetPhysics().createScene(sceneDesc);
		newComponent.m_defaultMat = PxGetPhysics().createMaterial(0.5f, 0.5f, 0.6f);
#if PHYSICS_DEBUG
		Physics::AddPhysicsWorld(_entity);
#endif
		physicsWorlds.emplace(_entity, PhysicsWorldInternalData{ 0u });

		ecs::add_component(_entity.GetValue(), newComponent);
	}

	template<>
	void RemoveComponent<Physics::World>(EntityID const _entity)
	{
		Physics::World* const oldComponent = ecs::get_component<Physics::World>(_entity.GetValue());
		ASSERT(oldComponent);
		oldComponent->m_scene->release();
		oldComponent->m_dispatcher; // how to release?

#if PHYSICS_DEBUG
		Physics::RemovePhysicsWorld(_entity);
#endif
		ASSERT(physicsWorlds.at(_entity).numBodies == 0u);
		physicsWorlds.erase(_entity);

		ecs::remove_component<Physics::World>(_entity.GetValue());
	}

	template<>
	void AddComponent(EntityID const _entity, Physics::RigidBodyDesc const& _desc)
	{
		Physics::RigidBody newComponent{};

		ASSERT(_desc.m_physicsWorld.IsValid());
		newComponent.m_physicsWorld = _desc.m_physicsWorld;
		Core::Physics::World& physicsWorld = Physics::GetWorld(newComponent.m_physicsWorld);

		// Select shape
		switch (_desc.m_shapeType)
		{
		case Physics::ShapeType::Box:
		{
			newComponent.m_geom = new physx::PxBoxGeometry(_desc.m_boxHalfDimensions);
			break;
		}
		case Physics::ShapeType::Sphere:
		{
			newComponent.m_geom = new physx::PxSphereGeometry(_desc.m_radius);
			break;
		}
		}

		btScalar const mass = std::max(_desc.m_mass, 0.0f);
		newComponent.m_isStatic = _desc.m_mass <= 0.0f;

		// Finalise RB
		if (newComponent.m_isStatic)
		{
			newComponent.m_body = physx::PxCreateStatic(PxGetPhysics(), _desc.m_startTransform.GetPhysxTransform(), *newComponent.m_geom, *physicsWorld.m_defaultMat);
		}
		else
		{
			physx::PxRigidDynamic* dynamic = physx::PxCreateDynamic(PxGetPhysics(), _desc.m_startTransform.GetPhysxTransform(), *newComponent.m_geom, *physicsWorld.m_defaultMat, 10.0f);
			dynamic->setAngularDamping(0.5f);
			dynamic->setLinearVelocity(physx::PxVec3(0));

			if (_desc.m_isKinematic)
			{
				dynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
			}
			newComponent.m_body = dynamic;
		}

		// Add to physics world
		physicsWorld.m_scene->addActor(*newComponent.m_body);

		physicsWorlds.at(newComponent.m_physicsWorld).numBodies++;

		// Add to ecs
		ecs::add_component(_entity.GetValue(), newComponent);
	}

	template<>
	void RemoveComponent<Physics::RigidBody>(EntityID const _entity)
	{
		Physics::RigidBody* const oldComponent = ecs::get_component<Physics::RigidBody>(_entity.GetValue());
		ASSERT(oldComponent);

		Core::Physics::World& physicsWorld = Physics::GetWorld(oldComponent->m_physicsWorld);
		physicsWorld.m_scene->removeActor(*oldComponent->m_body);

		physicsWorlds.at(oldComponent->m_physicsWorld).numBodies--;

		SafeDelete(oldComponent->m_geom);
		oldComponent->m_body->release();

		ecs::remove_component<Physics::World>(_entity.GetValue());
	}

	template<>
	void AddComponent(EntityID const _entity, Physics::CharacterControllerDesc const& _desc)
	{
		/*Physics::CharacterController newComponent{};

		ASSERT(_desc.m_physicsWorld.IsValid());
		newComponent.m_physicsWorld = _desc.m_physicsWorld;

		newComponent.m_halfHeight = _desc.m_halfHeight;
		newComponent.m_shape = new btCapsuleShape(_desc.m_radius, _desc.m_halfHeight);

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

		physicsWorlds.at(newComponent.m_physicsWorld).numBodies++;

		// Add to ecs
		ecs::add_component(_entity.GetValue(), newComponent);*/
	}

	template<>
	void RemoveComponent<Physics::CharacterController>(EntityID const _entity)
	{
		/*Physics::CharacterController* const oldComponent = ecs::get_component<Physics::CharacterController>(_entity.GetValue());
		ASSERT(oldComponent);

		Core::Physics::World& physicsWorld = Physics::GetWorld(oldComponent->m_physicsWorld);
		physicsWorld.m_dynamicsWorld->removeCollisionObject(oldComponent->m_body);

		physicsWorlds.at(oldComponent->m_physicsWorld).numBodies--;

		SafeDelete(oldComponent->m_motionState);
		SafeDelete(oldComponent->m_body);
		SafeDelete(oldComponent->m_shape);

		ecs::remove_component<Physics::World>(_entity.GetValue());*/
	}
}