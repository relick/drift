#pragma once

#include "common.h"

#include <ecs/ecs.h>
#include "Entity.h"
#include "scenes/Scene.h"

#include <absl/container/flat_hash_map.h>
#include <absl/strings/str_format.h>
#include <memory>

namespace Core
{
	namespace detail
	{
		template<typename T>
		concept ValidComponent = !requires{ typename std::remove_cvref_t<T>::_not_a_component; };
		template<typename T>
		concept DoesNotNeedInitialiser = ValidComponent<T> && !requires{ typename std::remove_cvref_t<T>::_initialiser_only; };
	}

	EntityID CreateEntity();
	EntityID CreatePersistentEntity();
	void DestroyEntity(EntityID _entity);
	void DestroyAllEntities();
	void ChangeEntityPersistence(EntityID _entity, bool _keepBetweenScenes);

	template<detail::ValidComponent T_Component>
	void CleanupComponent(EntityID const _entity);

	template<detail::ValidComponent T_Component>
	void RemoveComponent(EntityID const _entity);
	
	// Helpers for destroying components based on type hash when destroying entities
	struct ComponentDestroyerBase
	{
		virtual ~ComponentDestroyerBase() = default;
		virtual void CleanupComponent(EntityID _entity) const = 0;
		virtual void RemoveComponent(EntityID _entity) const = 0;
	};

	template<typename T_Component>
	struct ComponentDestroyer : ComponentDestroyerBase
	{
		~ComponentDestroyer() override = default;
		void CleanupComponent(EntityID _entity) const override
		{
			Core::CleanupComponent<T_Component>(_entity);
		}
		void RemoveComponent(EntityID _entity) const override
		{
			Core::RemoveComponent<T_Component>(_entity);
		}
	};

	namespace EntityManagement
	{
		// part of ecs::detail so not explicitly part of the API... but there's no point rewriting good code if we can just reuse it
		using ComponentHash = ecs::detail::type_hash;
		template<detail::ValidComponent T_Component>
		constexpr ComponentHash GetComponentHash() { return ecs::detail::get_type_hash<T_Component>(); }

		extern absl::flat_hash_map<ComponentHash, std::unique_ptr<ComponentDestroyerBase>> g_destroyers;

		template<detail::ValidComponent T_Component>
		void EnsureDestroyer(ComponentHash _hash)
		{
			auto it = g_destroyers.find(_hash);
			if (it == g_destroyers.end())
			{
				g_destroyers[_hash] = std::unique_ptr<ComponentDestroyerBase>(new ComponentDestroyer<T_Component>());
			}
		}
		
		void AddComponentHash(EntityID _entity, ComponentHash _hash);
		void RemoveComponentHash(EntityID _entity, ComponentHash _hash);

		template<detail::ValidComponent T_Component>
		void ComponentAdded(EntityID _entity)
		{
			ComponentHash const hash = GetComponentHash<T_Component>();
			EnsureDestroyer<T_Component>(hash);
			AddComponentHash(_entity, hash);

			kaLog(absl::StrFormat("Entity %d requested component %s to be added", _entity.GetDebugValue(), typeid(T_Component).name()));
		}
		template<detail::ValidComponent T_Component>
		void ComponentRemoved(EntityID _entity)
		{
			ComponentHash const hash = GetComponentHash<T_Component>();
			RemoveComponentHash(_entity, hash);

			kaLog(absl::StrFormat("Entity %d requested component %s to be removed", _entity.GetDebugValue(), typeid(T_Component).name()));
		}
		void CommitChanges();

		void TransitionScene(std::unique_ptr<Core::Scene::BaseScene> _nextScene);
	}

	// functions used by elements of the ECS itself and not by game code
	// wrapping the underlying ecs lib makes it possible to actually destroy entities, keep track of what to serialise, etc.
	namespace ECS
	{
		// Wraps ecs::add_component, shouldn't be used directly except by specialisations of AddComponent
		template <detail::ValidComponent T_Component>
		void AddComponent(EntityID const _entity, T_Component const& _component)
		{
			EntityManagement::ComponentAdded<T_Component>(_entity);
			ecs::add_component(Core::detail::AccessECSID(_entity), static_cast<std::remove_const_t<T_Component>>(_component));
		}

		// Wraps ecs::remove_component, shouldn't be used directly except by specialisations of RemoveComponent
		template<detail::ValidComponent T_Component>
		void RemoveComponent(EntityID const _entity)
		{
			EntityManagement::ComponentRemoved<T_Component>(_entity);
			ecs::remove_component<T_Component>(Core::detail::AccessECSID(_entity));
		}

		// Wrap ecs::commit_changes
		inline void CommitChanges()
		{
			EntityManagement::CommitChanges();
			ecs::commit_changes();
		}

		inline void Update()
		{
			CommitChanges();
			ecs::run_systems();
		}
	}

	// Basic AddComponent. This allows for specialisation for components that want to initialise things
	template<detail::DoesNotNeedInitialiser T_Component>
	void AddComponent(EntityID const _entity, T_Component const& _component)
	{
		ECS::AddComponent(_entity, _component);
	}

	// Expanded form for convenience - don't specialise this.
	template<detail::DoesNotNeedInitialiser T_FirstComponent, detail::DoesNotNeedInitialiser... T_Components>
	void AddComponents(EntityID const _entity, T_FirstComponent const& _firstComponent, T_Components const&... _components)
	{
		AddComponent(_entity, _firstComponent);
		(AddComponent(_entity, _components), ...);
	}

	// Basic CleanupComponent. This allows for specialisation for components that want to destroy things
	// pre-condition: entity has the component.
	template<detail::ValidComponent T_Component>
	void CleanupComponent(EntityID const _entity)
	{
	}

	// Basic RemoveComponent. Do not specialise
	// pre-condition: entity has the component.
	template<detail::ValidComponent T_Component>
	void RemoveComponent(EntityID const _entity)
	{
		ECS::RemoveComponent<T_Component>(_entity);
	}

	// Wrap get_component. Nothing special
	template<detail::ValidComponent T_Component>
	T_Component* GetComponent(EntityID const _entity)
	{
		return ecs::get_component<T_Component>(Core::detail::AccessECSID(_entity));
	}

	template<detail::ValidComponent T_Component>
	T_Component& GetGlobalComponent()
	{
		return ecs::get_global_component<T_Component>();
	}

	// Wrap ecs::make_system
	template<int t_Group, typename T_SystemFn, typename T_SortFn = std::nullptr_t>
	auto& MakeSystem(T_SystemFn _sysFn, T_SortFn _sortFn = nullptr)
	{
		return ecs::make_system<ecs::opts::group<t_Group>>(_sysFn, _sortFn);
	}

	template<int t_Group, typename T_SystemFn, typename T_SortFn = std::nullptr_t>
	auto& MakeSerialSystem(T_SystemFn _sysFn, T_SortFn _sortFn = nullptr)
	{
		return ecs::make_system<ecs::opts::group<t_Group>, ecs::opts::not_parallel>(_sysFn, _sortFn);
	}


	namespace Scene
	{
		namespace detail
		{
			template<typename T>
			concept SceneType = std::derived_from<T, Core::Scene::BaseScene>;
		}

		template<detail::SceneType T_Scene>
		void NextScene()
		{
			EntityManagement::TransitionScene(std::unique_ptr<BaseScene>(new T_Scene{}));
		}
	}
}