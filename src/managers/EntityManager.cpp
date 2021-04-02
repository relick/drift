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

		absl::flat_hash_map<ComponentHash, std::unique_ptr<ComponentDestroyerBase>> g_destroyers{};

		static absl::flat_hash_map<EntityID, std::vector<ComponentChange>> g_uncommittedComponentChanges{};
		static absl::flat_hash_map<EntityID, std::vector<ComponentHash>> g_committedComponents{};
		static std::recursive_mutex g_componentChangeMutex;

		static absl::flat_hash_set<EntityID> g_activeEntities;
		static std::shared_mutex g_entityMutex;

		static ComponentDestroyerBase const* GetDestroyer
		(
			ComponentHash _hash
		)
		{
			auto compDestroyerI = g_destroyers.find(_hash);
			kaAssert(compDestroyerI != g_destroyers.end(), "tried to destroy a component that is missing a destroyer");
			return compDestroyerI->second.get();
		}

		static bool IsActiveEntity
		(
			EntityID _entity
		)
		{
			std::shared_lock lock(g_entityMutex);
			return g_activeEntities.contains(_entity);
		}

		static std::vector<ComponentChange>::iterator FindComponentChange
		(
			std::vector<ComponentChange>& _compList,
			ComponentHash _hash
		)
		{
			return std::find_if(_compList.begin(), _compList.end(), [&_hash](auto const& _c) { return _c.m_hash == _hash; });
		}

		static std::vector<ComponentHash>::iterator FindComponent
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
			std::scoped_lock lock(g_componentChangeMutex);
			// slow assert
			kaAssert(FindComponentChange(g_uncommittedComponentChanges[_entity], _hash) == g_uncommittedComponentChanges[_entity].end(), "do not try to add/remove the same component to an entity more than once a frame");
			g_uncommittedComponentChanges[_entity].emplace_back(_hash, true);
		}

		void RemoveComponentHash
		(
			EntityID _entity,
			ComponentHash _hash
		)
		{
			std::scoped_lock lock(g_componentChangeMutex);

			// slow assert
			kaAssert(FindComponentChange(g_uncommittedComponentChanges[_entity], _hash) == g_uncommittedComponentChanges[_entity].end(), "do not try to add/remove the same component to an entity more than once a frame");
			g_uncommittedComponentChanges[_entity].emplace_back(_hash, false);
		}

		void CommitChanges()
		{
			std::scoped_lock lock(g_componentChangeMutex);

			for (auto const& [entity, compChangeList] : g_uncommittedComponentChanges)
			{
				auto& componentList = g_committedComponents[entity];
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
			g_uncommittedComponentChanges.clear();

			for (auto entityI = g_committedComponents.begin(); entityI != g_committedComponents.end();)
			{
				auto entityCopyI = entityI++;
				if (entityCopyI->second.empty())
				{
					g_committedComponents.erase(entityCopyI);
				}
			}
		}

		static void AddActiveEntity
		(
			EntityID _entity
		)
		{
			std::unique_lock lock(g_entityMutex);
			g_activeEntities.insert(_entity);

			kaLog(absl::StrFormat("Entity %d was created", _entity.GetDebugValue()));
		}

		static void RemoveActiveEntity
		(
			EntityID _entity
		)
		{
			std::unique_lock lock(g_entityMutex);
			g_activeEntities.erase(_entity);

			kaLog(absl::StrFormat("Entity %d was killed", _entity.GetDebugValue()));
		}
		static void RemoveAllActiveEntities()
		{
			std::unique_lock lock(g_entityMutex);
			g_activeEntities.clear();

			kaLog("-- All entities killed --");
		}

		static void DestroyEntityAndComponents
		(
			EntityID _entity
		)
		{
			kaAssert(IsActiveEntity(_entity), "tried to destroy dead entity!");

			// this function is what all this effort is for.
			std::scoped_lock lock(g_componentChangeMutex);

			auto entityI = g_committedComponents.find(_entity);
			kaAssert(entityI != g_committedComponents.end(), "tried to destroy non-existent entity");

			for (auto compHashI = entityI->second.rbegin(); compHashI != entityI->second.rend(); ++compHashI)
			{
				GetDestroyer(*compHashI)->RemoveComponent(_entity);
			}
			CommitChanges();
		}

		static void DestroyAllEntities()
		{
			{
				std::shared_lock lock(g_entityMutex);

				while (!g_activeEntities.empty())
				{
					auto maxI = std::max_element(g_activeEntities.begin(), g_activeEntities.end());

					DestroyEntityAndComponents(*maxI);

					g_activeEntities.erase(maxI);
				}
			}
			RemoveAllActiveEntities();
		}
	}

	static EntityID::CoreType g_nextID = 0;

	EntityID CreateEntity()
	{
		EntityID newID{ g_nextID++ };
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