#include "UIComponents.h"

#include "managers/ResourceManager.h"
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
		if ( loadingScreen.m_fullScreenSprite.IsNull() )
		{
			bool const success = Core::Resource::LoadSprite( "assets/sprites/loading/loading.spr", loadingScreen.m_fullScreenSprite );
			kaAssert( success );
		}

		ECS::AddComponent( _entity, loadingScreen );
	}
}