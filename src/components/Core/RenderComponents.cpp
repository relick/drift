#include "RenderComponents.h"

#include "managers/ResourceManager.h"

namespace Core
{
	template<>
	void AddComponent(EntityID const _entity, Render::ModelDesc const& _desc)
	{
		Render::Model newComponent{};

		bool const loaded = Core::Resource::LoadModel(_desc.m_filePath, newComponent.m_modelID);

		kaAssert(loaded, "couldn't load model, not adding component");
		if (loaded)
		{
			// Add to ecs
			Core::ECS::AddComponent(_entity, newComponent);
		}
	}

	template<>
	void AddComponent(EntityID const _entity, Render::SkyboxDesc const& _desc)
	{
		Render::Skybox newComponent{};

		bool const loaded = Core::Resource::LoadCubemap(_desc.m_cubemapPath, newComponent.m_cubemapID);

		kaAssert(loaded, "couldn't load cubemap for skybox, not adding component");
		if (loaded)
		{
			// Add to ecs
			Core::ECS::AddComponent(_entity, newComponent);
		}
	}

	template<>
	void AddComponent(EntityID const _entity, Render::SpriteDesc const& _desc)
	{
		Render::Sprite newComponent{};

		bool const loaded = Core::Resource::LoadSprite(_desc.m_filePath, newComponent.m_spriteID);

		kaAssert(loaded, "couldn't load model, not adding component");
		if (loaded)
		{
			// Add to ecs
			Core::ECS::AddComponent(_entity, newComponent);
		}
	}
}