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

		struct EntityData
		{
			absl::flat_hash_set<EntityID> m_active;
			absl::flat_hash_set<EntityID> m_sceneActive;

			bool IsActiveEntity( EntityID _entity ) const
			{
				return m_active.contains( _entity );
			}

			void AddActiveEntity
			(
				EntityID _entity,
				bool _persistent
			)
			{
				m_active.insert( _entity );
				if ( !_persistent )
				{
					m_sceneActive.insert( _entity );
				}

				kaLog( std::format( "Entity {:d} was created", _entity.GetDebugValue() ) );
			}

			void RemoveActiveEntity
			(
				EntityID _entity
			)
			{
				m_active.erase( _entity );
				m_sceneActive.erase( _entity );

				kaLog( std::format( "Entity {:d} was killed", _entity.GetDebugValue() ) );
			}

			void RemoveAllActiveEntities()
			{
				m_active.clear();
				m_sceneActive.clear();

				kaLog( "-- All entities killed --" );
			}
		};
		using EntityDataMutex = SharedMutex< EntityData >;
		using EntityDataWriteGuard = EntityDataMutex::WriteGuard;
		static EntityDataMutex g_entityData;

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

		static bool IsActiveEntity
		(
			EntityID _entity
		)
		{
			return g_entityData.Read()->IsActiveEntity( _entity );
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

		static void AddActiveEntity
		(
			EntityID _entity,
			bool _persistent
		)
		{
			g_entityData.Write()->AddActiveEntity( _entity, _persistent );
		}

		static void RemoveActiveEntity
		(
			EntityID _entity
		)
		{
			g_entityData.Write()->RemoveActiveEntity( _entity );
		}

		static void RemoveAllActiveEntities()
		{
			g_entityData.Write()->RemoveAllActiveEntities();
		}


		static void ChangeEntityPersistence
		(
			EntityID _entity,
			bool _keepBetweenScenes
		)
		{
			auto entityAccess = g_entityData.Write();
			kaAssert(entityAccess->IsActiveEntity(_entity), "Tried to change persistence of inactive entity!");

			if (_keepBetweenScenes)
			{
				entityAccess->m_sceneActive.erase(_entity);
			}
			else
			{
				entityAccess->m_sceneActive.emplace(_entity);
			}
		}

		static void DestroyEntityAndComponents
		(
			EntityDataWriteGuard& _entityAccess,
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
				for (auto const& entity : _entityAccess->m_active)
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
				kaAssert(_entityAccess->IsActiveEntity(*entityI), "tried to destroy dead entity!");

				auto commCompI = componentChangeAccess->m_committed.find(*entityI);
				kaAssert(commCompI != componentChangeAccess->m_committed.end(), "tried to destroy non-existent entity");

				for (auto compHashI = commCompI->second.rbegin(); compHashI != commCompI->second.rend(); ++compHashI)
				{
					GetDestroyer(*compHashI)->RemoveComponent(*entityI);
				}
				CommitChanges();

				_entityAccess->RemoveActiveEntity(*entityI);
			}
		}

		static void DestroyAllEntities()
		{
			auto entityAccess = g_entityData.Write();

			while (!entityAccess->m_active.empty())
			{
				auto maxI = std::max_element(entityAccess->m_active.begin(), entityAccess->m_active.end());

				DestroyEntityAndComponents(entityAccess, *maxI);
			}

			entityAccess->RemoveAllActiveEntities();
		}

		static void DestroyAllSceneEntities()
		{
			auto entityAccess = g_entityData.Write();

			while (!entityAccess->m_sceneActive.empty())
			{
				auto maxI = std::max_element(entityAccess->m_sceneActive.begin(), entityAccess->m_sceneActive.end());

				DestroyEntityAndComponents(entityAccess, *maxI);
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
		auto entityAccess = EntityManagement::g_entityData.Write();
		EntityManagement::DestroyEntityAndComponents(entityAccess, _entity);
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