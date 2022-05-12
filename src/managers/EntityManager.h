#pragma once

#include "common.h"

#include <ecs/ecs.h>
#include "Entity.h"
#include "MT_Only.h"
#include "SystemOrdering.h"
#include "scenes/Scene.h"

#include <absl/container/flat_hash_map.h>

#include <format>
#include <memory>
#include <type_traits>

#define ENTITY_LOGGING_ENABLED 0

namespace Core
{
	namespace detail
	{
		template<typename T>
		concept ValidComponent = !requires{ typename std::remove_cvref_t<T>::_not_a_component; };
		template<typename T>
		concept DoesNotNeedInitialiser = ValidComponent<T> && !requires{ typename std::remove_cvref_t<T>::_initialiser_only; };

		ecs::runtime& GetEcsRuntime();
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

#if ENTITY_LOGGING_ENABLED
			kaLog(std::format("Entity {:d} requested component {:s} to be added", _entity.GetDebugValue(), typeid(T_Component).name()));
#endif
		}
		template<detail::ValidComponent T_Component>
		void ComponentRemoved(EntityID _entity)
		{
			ComponentHash const hash = GetComponentHash<T_Component>();
			RemoveComponentHash(_entity, hash);

#if ENTITY_LOGGING_ENABLED
			kaLog(std::format("Entity {:d} requested component {:s} to be removed", _entity.GetDebugValue(), typeid(T_Component).name()));
#endif
		}
		void CommitChanges();

		void TransitionScene(std::shared_ptr<Core::Scene::BaseScene> const& _nextScene);
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
			detail::GetEcsRuntime().add_component(Core::detail::AccessECSID(_entity), static_cast<std::remove_const_t<T_Component>>(_component));
		}

		// Wraps ecs::remove_component, shouldn't be used directly.
		template<detail::ValidComponent T_Component>
		void RemoveComponent(EntityID const _entity)
		{
			EntityManagement::ComponentRemoved<T_Component>(_entity);
			detail::GetEcsRuntime().remove_component<T_Component>(Core::detail::AccessECSID(_entity));
		}

		// Wrap ecs::commit_changes
		inline void CommitChanges()
		{
			EntityManagement::CommitChanges();
			detail::GetEcsRuntime().commit_changes();
		}

		inline void Update()
		{
			CommitChanges();
			detail::GetEcsRuntime().run_systems();
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
		return detail::GetEcsRuntime().get_component<T_Component>(Core::detail::AccessECSID(_entity));
	}

	template<detail::ValidComponent T_Component>
	T_Component& GetGlobalComponent()
	{
		return detail::GetEcsRuntime().get_global_component<T_Component>();
	}

	namespace detail
	{

	template< typename T >
	inline constexpr bool LambdaMT_Only = false;

	template< typename T_Ret, typename T_Class, typename... T_Args >
	inline constexpr bool LambdaMT_Only< T_Ret( T_Class::* )( T_Args... ) const > = ( std::is_same_v< Core::MT_Only&, T_Args > || ... );

	template< typename T >
	inline constexpr bool FuncMT_Only = false;

	template< typename T_Ret, typename... T_Args >
	inline constexpr bool FuncMT_Only< T_Ret( * )( T_Args... ) > = ( std::is_same_v< Core::MT_Only&, T_Args > || ... );

	template< typename T_SystemFn >
	constexpr bool AtLeastOneArgMT_Only()
	{
		if constexpr ( ecs::detail::type_is_function< T_SystemFn > )
		{
			return FuncMT_Only< T_SystemFn >;
		}
		else if constexpr ( ecs::detail::type_is_lambda< T_SystemFn > )
		{
			return LambdaMT_Only< decltype( &T_SystemFn::operator() ) >;
		}
		return false;
	}

	}

	// Wrap ecs::make_system
	template<int32 t_Group, typename T_SystemFn, typename T_SortFn = std::nullptr_t>
	auto& MakeSystem(T_SystemFn _sysFn, T_SortFn _sortFn = nullptr)
	{
		if constexpr ( Sys::c_mtOnlySysOrdering[ t_Group ] )
		{
			static_assert( detail::AtLeastOneArgMT_Only< T_SystemFn >(), "System tried to be added in Main Thread-only section without Core::MT_Only& parameter." );
		}
		else
		{
			static_assert( !detail::AtLeastOneArgMT_Only< T_SystemFn >(), "System had a Core::MT_Only& parameter but isn't in a Main Thread-only section." );
		}
		return detail::GetEcsRuntime().make_system< ecs::opts::group< t_Group > >( _sysFn, _sortFn );
	}

	template<int32 t_Group, typename T_SystemFn, typename T_SortFn = std::nullptr_t>
	auto& MakeSerialSystem(T_SystemFn _sysFn, T_SortFn _sortFn = nullptr)
	{
		if constexpr ( Sys::c_mtOnlySysOrdering[ t_Group ] )
		{
			static_assert( detail::AtLeastOneArgMT_Only< T_SystemFn >(), "System tried to be added in Main Thread-only section without Core::MT_Only& parameter." );
		}
		else
		{
			static_assert( !detail::AtLeastOneArgMT_Only< T_SystemFn >(), "System had a Core::MT_Only& parameter but isn't in a Main Thread-only section." );
		}
		return detail::GetEcsRuntime().make_system< ecs::opts::group< t_Group >, ecs::opts::not_parallel >( _sysFn, _sortFn );
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
			EntityManagement::TransitionScene(std::shared_ptr<BaseScene>(new T_Scene{}));
		}

		inline void NextScene( std::shared_ptr<BaseScene> const& _nextScene )
		{
			EntityManagement::TransitionScene( _nextScene );
		}
	}
}