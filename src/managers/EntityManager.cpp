#include "EntityManager.h"

#include "components.h"
#include "common/Mutex.h"

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>


ecs::runtime& Core::detail::GetEcsRuntime()
{
	// FIXME: love some hacky statics. I'll refactor everything later.
	static ecs::runtime globalRuntime;
	return globalRuntime;
}

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

		struct ComponentChangeData
		{
			absl::flat_hash_map<EntityID, std::vector<ComponentChange>> m_uncommittedChanges{};
			absl::flat_hash_map<EntityID, std::vector<ComponentHash>> m_committed{};
		};
		static RecursiveMutex< ComponentChangeData > g_componentChangeData;

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
			std::shared_lock<std::shared_mutex> lock{ g_entityMutex };
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
			auto componentChangeAccess = g_componentChangeData.Write();
			// slow assert
			kaAssert(FindComponentChange(componentChangeAccess->m_uncommittedChanges[_entity], _hash) == componentChangeAccess->m_uncommittedChanges[_entity].end(), "do not try to add/remove the same component to an entity more than once a frame");
			componentChangeAccess->m_uncommittedChanges[_entity].emplace_back(_hash, true);
		}

		void RemoveComponentHash
		(
			EntityID _entity,
			ComponentHash _hash
		)
		{
			auto componentChangeAccess = g_componentChangeData.Write();

			// slow assert
			kaAssert(FindComponentChange(componentChangeAccess->m_uncommittedChanges[_entity], _hash) == componentChangeAccess->m_uncommittedChanges[_entity].end(), "do not try to add/remove the same component to an entity more than once a frame");
			componentChangeAccess->m_uncommittedChanges[_entity].emplace_back(_hash, false);
		}

		void CommitChanges()
		{
			auto componentChangeAccess = g_componentChangeData.Write();

			for (auto const& [entity, compChangeList] : componentChangeAccess->m_uncommittedChanges)
			{
				auto& componentList = componentChangeAccess->m_committed[entity];
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
			componentChangeAccess->m_uncommittedChanges.clear();

			for (auto entityI = componentChangeAccess->m_committed.begin(); entityI != componentChangeAccess->m_committed.end();)
			{
				auto entityCopyI = entityI++;
				if (entityCopyI->second.empty())
				{
					componentChangeAccess->m_committed.erase(entityCopyI);
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

			kaLog(std::format("Entity {:d} was created", _entity.GetDebugValue()));
		}

		static void AddActiveEntity
		(
			EntityID _entity,
			bool _persistent
		)
		{
			std::unique_lock<std::shared_mutex> lock{ g_entityMutex };
			AddActiveEntity_Unsafe(_entity, _persistent);
		}

		static void RemoveActiveEntity_Unsafe
		(
			EntityID _entity
		)
		{
			g_activeEntities.erase(_entity);
			g_sceneActiveEntities.erase(_entity);

			kaLog(std::format("Entity {:d} was killed", _entity.GetDebugValue()));
		}

		static void RemoveActiveEntity
		(
			EntityID _entity
		)
		{
			std::unique_lock<std::shared_mutex> lock{ g_entityMutex };
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
			std::unique_lock<std::shared_mutex> lock{ g_entityMutex };
			RemoveAllActiveEntities_Unsafe();
		}


		static void ChangeEntityPersistence
		(
			EntityID _entity,
			bool _keepBetweenScenes
		)
		{
			std::unique_lock<std::shared_mutex> lock{ g_entityMutex };
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
			auto componentChangeAccess = g_componentChangeData.Write();

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

				auto commCompI = componentChangeAccess->m_committed.find(*entityI);
				kaAssert(commCompI != componentChangeAccess->m_committed.end(), "tried to destroy non-existent entity");

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
			std::unique_lock<std::shared_mutex> lock{ g_entityMutex };

			while (!g_activeEntities.empty())
			{
				auto maxI = std::max_element(g_activeEntities.begin(), g_activeEntities.end());

				DestroyEntityAndComponents(*maxI);
			}

			RemoveAllActiveEntities_Unsafe();
		}

		static void DestroyAllSceneEntities()
		{
			std::unique_lock<std::shared_mutex> lock{ g_entityMutex };

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

			if (_nextScene)
			{
				g_currentScene = std::move(_nextScene);
				g_currentScene->Setup();
			}
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