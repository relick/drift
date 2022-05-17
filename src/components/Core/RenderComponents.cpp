#include "RenderComponents.h"

#include "managers/ResourceManager.h"
#include "managers/RenderManager.h"

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

		bool loaded = true;
		if ( std::string const* filePath = std::get_if<std::string>( &_desc.m_spriteInit ); filePath != nullptr )
		{
			loaded = Core::Resource::LoadSprite( *filePath, newComponent.m_spriteID );
			kaAssert( loaded, "couldn't load sprite, not adding component" );
		}
		else
		{
			newComponent.m_spriteID = std::get<Resource::SpriteID>( _desc.m_spriteInit );
		}

		if (loaded)
		{
			// Add to render manager
			newComponent.m_spriteSceneID = Core::Render::AddSpriteToScene( newComponent.m_spriteID, _desc.m_initTrans, _desc.m_initFlags );
			
			// Add to ecs
			Core::ECS::AddComponent(_entity, newComponent);
		}
	}

	template<>
	void CleanupComponent<Render::Sprite>( EntityID const _entity )
	{
		Render::Sprite* const oldComponent = Core::GetComponent<Render::Sprite>(_entity);
		kaAssert( oldComponent );

		Core::Render::RemoveSpriteFromScene( oldComponent->m_spriteSceneID );
	}
}