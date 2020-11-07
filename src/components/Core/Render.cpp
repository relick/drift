#include "Render.h"

#include "managers/Resources.h"

#include <iostream>

namespace Core
{
	template<>
	void AddComponent(EntityID const _entity, Render::ModelDesc const& _desc)
	{
		Render::Model newComponent{};

		bool const loaded = Core::Resource::LoadModel(_desc.m_filePath, newComponent.m_modelID);

		if (loaded)
		{
			// Add to ecs
			ecs::add_component(_entity.GetValue(), newComponent);
		}
		else
		{
			std::cout << "couldn't load model, not adding component" << std::endl;
		}
	}

	template<>
	void RemoveComponent<Render::Model>(EntityID const _entity)
	{
		Render::Model* const oldComponent = ecs::get_component<Render::Model>(_entity.GetValue());
		ASSERT(oldComponent);

		// TODO cleanup shit

		ecs::remove_component<Render::Model>(_entity.GetValue());
	}
}