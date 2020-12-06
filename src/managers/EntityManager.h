#pragma once

#include <ecs/ecs.h>
#include "Entity.h"

namespace Core
{
	extern ecs::entity_id nextID;

	EntityID CreateEntity();

	namespace detail
	{
#define use_initialiser struct _initialiser_only {}
		template<typename T>
		concept DoesNotNeedInitialiser = !requires{ typename std::remove_cvref_t<T>::_initialiser_only; };
	}

	// Wrap add_component. This allows for specialisation for components that want to initialise things
	template<detail::DoesNotNeedInitialiser T_Component>
	void AddComponent(EntityID const _entity, T_Component const& _component)
	{
		ecs::add_component(_entity.GetValue(), static_cast<std::remove_const_t<T_Component>>(_component));
	}

	// Expand it here - add_component does this anyway so shouldn't be any different.
	template<detail::DoesNotNeedInitialiser T_FirstComponent, detail::DoesNotNeedInitialiser... T_Components>
	void AddComponents(EntityID const _entity, T_FirstComponent const& _firstComponent, T_Components const&... _components)
	{
		AddComponent(_entity, _firstComponent);
		(AddComponent(_entity, _components), ...);
	}

	// Wrap remove_component. This allows for specialisation for components that want to destroy things
	// pre-condition: entity has the component.
	template<typename T_Component>
	void RemoveComponent(EntityID const _entity)
	{
		ecs::remove_component<T_Component>(_entity.GetValue());
	}

}