#include "RenderComponents.h"

#include "managers/ResourceManager.h"

namespace Core
{
	template<>
	void AddComponent(EntityID const _entity, Render::ModelDesc const& _desc)
	{
		Render::Model newComponent{};

		bool const loaded = Core::Resource::LoadModel(_desc.m_filePath, newComponent.m_modelID);

		ASSERT(loaded, "couldn't load model, not adding component");
		if (loaded)
		{
			// Add to ecs
			ecs::add_component(_entity.GetValue(), newComponent);
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