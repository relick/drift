#include "EntityManager.h"

#include "components.h"

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
		static absl::flat_hash_set<EntityID> g_sceneActiveEntities;
		static std::shared_mutex g_entityMutex;

		static std::unique_ptr<Core::Scene::BaseScene> g_currentScene;

		static ComponentDestroyerBase const* GetDestroyer
		(
			ComponentHash _hash
		)
		{
			auto compDestroyerI = g_destroyers.find(_hash);
			kaAssert(compDestroyerI != g_destroyers.end(), "tried to destroy a component that is missing a destroyer");
			return compDestroyerI->second.get();
		}

		static bool IsActiveEntity_Unsafe
		(
			EntityID _entity
		)
		{
			return g_activeEntities.contains(_entity);
		}

		static bool IsActiveEntity
		(
			EntityID _entity
		)
		{
			std::shared_lock lock(g_entityMutex);
			return IsActiveEntity_Unsafe(_entity);
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

		static void AddActiveEntity_Unsafe
		(
			EntityID _entity,
			bool _persistent
		)
		{
			g_activeEntities.insert(_entity);
			if (!_persistent)
			{
				g_sceneActiveEntities.insert(_entity);
			}

			kaLog(absl::StrFormat("Entity %d was created", _entity.GetDebugValue()));
		}

		static void AddActiveEntity
		(
			EntityID _entity,
			bool _persistent
		)
		{
			std::unique_lock lock(g_entityMutex);
			AddActiveEntity_Unsafe(_entity, _persistent);
		}

		static void RemoveActiveEntity_Unsafe
		(
			EntityID _entity
		)
		{
			g_activeEntities.erase(_entity);
			g_sceneActiveEntities.erase(_entity);

			kaLog(absl::StrFormat("Entity %d was killed", _entity.GetDebugValue()));
		}

		static void RemoveActiveEntity
		(
			EntityID _entity
		)
		{
			std::unique_lock lock(g_entityMutex);
			RemoveActiveEntity_Unsafe(_entity);
		}

		static void RemoveAllActiveEntities_Unsafe()
		{
			g_activeEntities.clear();
			g_sceneActiveEntities.clear();

			kaLog("-- All entities killed --");
		}

		static void RemoveAllActiveEntities()
		{
			std::unique_lock lock(g_entityMutex);
			RemoveAllActiveEntities_Unsafe();
		}


		static void ChangeEntityPersistence
		(
			EntityID _entity,
			bool _keepBetweenScenes
		)
		{
			std::unique_lock lock(g_entityMutex);
			kaAssert(IsActiveEntity_Unsafe(_entity), "Tried to change persistence of inactive entity!");

			if (_keepBetweenScenes)
			{
				g_sceneActiveEntities.erase(_entity);
			}
			else
			{
				g_sceneActiveEntities.emplace(_entity);
			}
		}

		static void DestroyEntityAndComponents
		(
			EntityID _entity
		)
		{
			// this function is what all this effort is for.
			std::scoped_lock lock(g_componentChangeMutex);

			absl::InlinedVector<EntityID, 32> entitiesToDestroy;
			entitiesToDestroy.emplace_back(_entity);
			
			// fill vector until no more children are found
			for(usize entityToCheckI{ 0 }; entityToCheckI < entitiesToDestroy.size(); ++entityToCheckI)
			{
				for (auto const& entity : g_activeEntities)
				{
					if (auto const transform = Core::GetComponent<Core::Transform3D>(entity); transform && transform->m_parent.IsValid())
					{
						if (transform->m_parent == entitiesToDestroy[entityToCheckI])
						{
							entitiesToDestroy.emplace_back(entity);
						}
					}
				}
			}

			for (auto entityI = entitiesToDestroy.rbegin(); entityI != entitiesToDestroy.rend(); ++entityI)
			{
				kaAssert(IsActiveEntity_Unsafe(*entityI), "tried to destroy dead entity!");

				auto commCompI = g_committedComponents.find(*entityI);
				kaAssert(commCompI != g_committedComponents.end(), "tried to destroy non-existent entity");

				for (auto compHashI = commCompI->second.rbegin(); compHashI != commCompI->second.rend(); ++compHashI)
				{
					GetDestroyer(*compHashI)->RemoveComponent(*entityI);
				}
				CommitChanges();

				RemoveActiveEntity_Unsafe(*entityI);
			}
		}

		static void DestroyAllEntities()
		{
			std::unique_lock lock(g_entityMutex);

			while (!g_activeEntities.empty())
			{
				auto maxI = std::max_element(g_activeEntities.begin(), g_activeEntities.end());

				DestroyEntityAndComponents(*maxI);
			}

			RemoveAllActiveEntities_Unsafe();
		}

		static void DestroyAllSceneEntities()
		{
			std::unique_lock lock(g_entityMutex);

			while (!g_sceneActiveEntities.empty())
			{
				auto maxI = std::max_element(g_sceneActiveEntities.begin(), g_sceneActiveEntities.end());

				DestroyEntityAndComponents(*maxI);
			}
		}

		void TransitionScene
		(
			std::unique_ptr<Core::Scene::BaseScene> _nextScene
		)
		{
			DestroyAllSceneEntities();
			g_currentScene.reset();

			g_currentScene = std::move(_nextScene);
			g_currentScene->Setup();
		}
	}

	static EntityID::CoreType g_nextID = 0;

	EntityID CreateEntity()
	{
		EntityID newID{ g_nextID++ };
		EntityManagement::AddActiveEntity(newID, false);
		return newID;
	}

	EntityID CreatePersistentEntity()
	{
		EntityID newID{ g_nextID++ };
		EntityManagement::AddActiveEntity(newID, true);
		return newID;
	}

	void DestroyEntity
	(
		EntityID _entity
	)
	{
		EntityManagement::DestroyEntityAndComponents(_entity);
	}

	void DestroyAllEntities()
	{
		EntityManagement::DestroyAllEntities();
	}

	void ChangeEntityPersistence
	(
		EntityID _entity,
		bool _keepBetweenScenes
	)
	{
		EntityManagement::ChangeEntityPersistence(_entity, _keepBetweenScenes);
	}
}