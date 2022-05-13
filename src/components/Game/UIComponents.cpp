#include "UIComponents.h"

#include "managers/ResourceManager.h"
#include "managers/RenderManager.h"
#include "components/Core/ResourceComponents.h"

namespace Core
{
	template<>
	void AddComponent
	(
		EntityID const _entity,
		Game::UI::SceneLoadDesc const& _component
	)
	{
		Core::Resource::Preload preload;

		preload.m_firstResFile = _component.m_nextScene->GetPreloadFile();
		ECS::AddComponent( _entity, preload );

		Game::UI::LoadingScreen loadingScreen;
		loadingScreen.m_nextScene = _component.m_nextScene;

		Core::Resource::SpriteID fullScreenSprite;
		bool const success = Core::Resource::LoadSprite( "assets/sprites/loading/loading.spr", fullScreenSprite );

		if ( success )
		{
			Trans2D fullScreenSpriteTrans; // default will cover the whole screen.
			loadingScreen.m_fullScreenSprite = Core::Render::AddSpriteToScene( fullScreenSprite, fullScreenSpriteTrans );
		}
		else
		{
			kaError( "Failed to load loading screen sprite" );
			return;
		}

		ECS::AddComponent( _entity, loadingScreen );
	}

	template<>
	void CleanupComponent<Game::UI::LoadingScreen>( EntityID const _entity )
	{
		Game::UI::LoadingScreen* const oldComponent = Core::GetComponent<Game::UI::LoadingScreen>( _entity );
		kaAssert( oldComponent );

		Core::Render::RemoveSpriteFromScene( oldComponent->m_fullScreenSprite );
	}
}