#pragma once

#include <ecs/ecs.h>
#include "Entity.h"

namespace Core
{
	extern EntityID::CoreType nextID;

	EntityID CreateEntity();

	namespace detail
	{
#define use_initialiser struct _initialiser_only {}
		template<typename T>
		concept DoesNotNeedInitialiser = !requires{ typename std::remove_cvref_t<T>::_initialiser_only; };
	}

	// functions used by elements of the ECS itself and not by game code
	// wrapping the underlying ecs lib makes it possible to actually destroy entities, keep track of what to serialise, etc.
	namespace ECS
	{
		// Wraps ecs::add_component, shouldn't be used directly except by specialisations of AddComponent
		template <typename T_Component>
		void AddComponent(EntityID const _entity, T_Component const& _component)
		{
			ecs::add_component(Core::detail::AccessECSID(_entity), static_cast<std::remove_const_t<T_Component>>(_component));
		}

		// Wraps ecs::remove_component, shouldn't be used directly except by specialisations of RemoveComponent
		template<typename T_Component>
		void RemoveComponent(EntityID const _entity)
		{
			ecs::remove_component<T_Component>(Core::detail::AccessECSID(_entity));
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

	// Basic RemoveComponent. This allows for specialisation for components that want to destroy things
	// pre-condition: entity has the component.
	template<typename T_Component>
	void RemoveComponent(EntityID const _entity)
	{
		ECS::RemoveComponent<T_Component>(_entity);
	}

	// Wrap get_component. Nothing special
	template<typename T_Component>
	T_Component* GetComponent(EntityID const _entity)
	{
		return ecs::get_component<T_Component>(Core::detail::AccessECSID(_entity));
	}
}