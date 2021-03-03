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
			Core::ECS::AddComponent(_entity, newComponent);
		}
	}

	template<>
	void RemoveComponent<Render::Model>(EntityID const _entity)
	{
		Render::Model* const oldComponent = Core::GetComponent<Render::Model>(_entity);
		ASSERT(oldComponent);

		// TODO cleanup shit

		Core::ECS::RemoveComponent<Render::Model>(_entity);
	}
}