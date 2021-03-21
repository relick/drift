#include "EntityManager.h"

#include "components.h"

#include "CubeTest.h"

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <mutex>
#include <shared_mutex>

namespace Core
{
	namespace EntityManagement
	{
		struct ComponentChange
		{
			ComponentHash m_hash;
			bool m_added;

			ComponentChange(ComponentHash _hash, bool _added) : m_hash{ _hash }, m_added{ _added } {}
		};

		absl::flat_hash_map<ComponentHash, std::unique_ptr<ComponentDestroyerBase>> destroyers{};

		absl::flat_hash_map<EntityID, std::vector<ComponentChange>> uncommittedComponentChanges{};
		absl::flat_hash_map<EntityID, std::vector<ComponentHash>> committedComponents{};
		std::recursive_mutex componentChangeMutex;

		absl::flat_hash_set<EntityID> activeEntities;
		std::shared_mutex entityMutex;

		ComponentDestroyerBase const* GetDestroyer
		(
			ComponentHash _hash
		)
		{
			auto compDestroyerI = destroyers.find(_hash);
			kaAssert(compDestroyerI != destroyers.end(), "tried to destroy a component that is missing a destroyer");
			return compDestroyerI->second.get();
		}

		bool IsActiveEntity
		(
			EntityID _entity
		)
		{
			std::shared_lock lock(entityMutex);
			return activeEntities.contains(_entity);
		}

		std::vector<ComponentChange>::iterator FindComponentChange
		(
			std::vector<ComponentChange>& _compList,
			ComponentHash _hash
		)
		{
			return std::find_if(_compList.begin(), _compList.end(), [&_hash](auto const& _c) { return _c.m_hash == _hash; });
		}

		std::vector<ComponentHash>::iterator FindComponent
		(
			std::vector<ComponentHash>& _compList,
			ComponentHash _hash
		)
		{
			return std::find(_compList.begin(), _compList.end(), _hash);
		}

		void AddComponentHash
		(
			EntityID _entity,
			ComponentHash _hash
		)
		{
			kaAssert(IsActiveEntity(_entity), "tried to add components to dead entity!");
			std::scoped_lock lock(componentChangeMutex);
			// slow assert
			kaAssert(FindComponentChange(uncommittedComponentChanges[_entity], _hash) == uncommittedComponentChanges[_entity].end(), "do not try to add/remove the same component to an entity more than once a frame");
			uncommittedComponentChanges[_entity].emplace_back(_hash, true);
		}

		void RemoveComponentHash
		(
			EntityID _entity,
			ComponentHash _hash
		)
		{
			std::scoped_lock lock(componentChangeMutex);

			// slow assert
			kaAssert(FindComponentChange(uncommittedComponentChanges[_entity], _hash) == uncommittedComponentChanges[_entity].end(), "do not try to add/remove the same component to an entity more than once a frame");
			uncommittedComponentChanges[_entity].emplace_back(_hash, false);
		}

		void CommitChanges()
		{
			std::scoped_lock lock(componentChangeMutex);

			for (auto const& [entity, compChangeList] : uncommittedComponentChanges)
			{
				auto& componentList = committedComponents[entity];
				for (auto const& change : compChangeList)
				{
					if (change.m_added)
					{
						kaAssert(FindComponent(componentList, change.m_hash) == componentList.end(), "tried to add a component twice");
						componentList.emplace_back(change.m_hash);
					}
					else
					{
						auto compI = FindComponent(componentList, change.m_hash);
						kaAssert(compI != componentList.end(), "tried to remove a component twice");
						GetDestroyer(change.m_hash)->CleanupComponent(entity);
						componentList.erase(compI);
					}
				}
			}
			uncommittedComponentChanges.clear();

			for (auto entityI = committedComponents.begin(); entityI != committedComponents.end();)
			{
				auto entityCopyI = entityI++;
				if (entityCopyI->second.empty())
				{
					committedComponents.erase(entityCopyI);
				}
			}
		}

		void AddActiveEntity
		(
			EntityID _entity
		)
		{
			std::unique_lock lock(entityMutex);
			activeEntities.insert(_entity);

			kaLog(absl::StrFormat("Entity %d was created", _entity.GetDebugValue()));
		}

		void RemoveActiveEntity
		(
			EntityID _entity
		)
		{
			std::unique_lock lock(entityMutex);
			activeEntities.erase(_entity);

			kaLog(absl::StrFormat("Entity %d was killed", _entity.GetDebugValue()));
		}
		void RemoveAllActiveEntities()
		{
			std::unique_lock lock(entityMutex);
			activeEntities.clear();

			kaLog("-- All entities killed --");
		}

		void DestroyEntityAndComponents
		(
			EntityID _entity
		)
		{
			kaAssert(IsActiveEntity(_entity), "tried to destroy dead entity!");

			// this function is what all this effort is for.
			std::scoped_lock lock(componentChangeMutex);

			auto entityI = committedComponents.find(_entity);
			kaAssert(entityI != committedComponents.end(), "tried to destroy non-existent entity");

			for (auto compHashI = entityI->second.rbegin(); compHashI != entityI->second.rend(); ++compHashI)
			{
				GetDestroyer(*compHashI)->RemoveComponent(_entity);
			}
			CommitChanges();
		}

		void DestroyAllEntities()
		{
			{
				std::shared_lock lock(entityMutex);

				while (!activeEntities.empty())
				{
					auto maxI = std::max_element(activeEntities.begin(), activeEntities.end());

					DestroyEntityAndComponents(*maxI);

					activeEntities.erase(maxI);
				}
			}
			RemoveAllActiveEntities();
		}
	}

	EntityID::CoreType nextID = 0;

	EntityID CreateEntity()
	{
		EntityID newID{ nextID++ };
		EntityManagement::AddActiveEntity(newID);
		return newID;
	}

	void DestroyEntity
	(
		EntityID _entity
	)
	{
		EntityManagement::DestroyEntityAndComponents(_entity);
		EntityManagement::RemoveActiveEntity(_entity);
	}

	void DestroyAllEntities()
	{
		EntityManagement::DestroyAllEntities();
	}

	namespace Scene
	{
		void AddDemoScene()
		{
			Core::EntityID const camera = Core::CreateEntity();
			Core::EntityID const character = Core::CreateEntity();
			fTrans const characterTrans{ fQuatIdentity(), fVec3(2.0f, 0.0f, 0.0f) };
			Core::AddComponent(character, Core::Transform3D(characterTrans));
			{
				Core::Physics::CharacterControllerDesc ccDesc{};
				ccDesc.m_viewObject = camera;
				ccDesc.m_halfHeight = 0.9f;
				ccDesc.m_radius = 0.5f;
				ccDesc.m_mass = 80.0f;
				ccDesc.m_startTransform = characterTrans;
				ccDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

				Core::AddComponent(character, ccDesc);
			}

			Core::AddComponent(camera, Core::Transform3D(fQuatIdentity(), fVec3(0.0f, 0.8f, 0.0f), character));
			Core::AddComponent(camera, Core::Render::MainCamera3D());
			Core::AddComponent(camera, Core::Render::DebugCameraControl());
			Core::AddComponent(camera, Game::Player::MouseLook());

			CubeTestEntities();
		}
	}
}