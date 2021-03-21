#include "UISystems.h"

#include "components.h"
#include "systems.h"
#include "SystemOrdering.h"

#include "managers/EntityManager.h"
#include "managers/RenderManager.h"
#include "managers/ResourceManager.h"
#include "managers/TextManager.h"

#include <absl/strings/str_format.h>

namespace Game::UI
{
	void Setup()
	{
		Core::MakeSystem<Sys::UI_UPDATE>([](Core::EntityID::CoreType _entity, Game::UI::LoadingScreen& _loadingScreen)
		{
			auto const preload = Core::GetComponent<Core::Resource::Preload>(_entity);
			if (preload == nullptr)
			{
				Core::DestroyEntity(_entity);
				Core::Scene::AddDemoScene();
				return;
			}
			if (preload->m_currentLoadingIndex < preload->m_filesToLoad.size())
			{
				_loadingScreen.m_nextLoadedFilename = preload->m_filesToLoad[preload->m_currentLoadingIndex].m_filePath;
			}
			_loadingScreen.m_currentlyLoaded = preload->m_currentLoadingIndex;
			_loadingScreen.m_totalToLoad = preload->m_filesToLoad.size();
		});

		Core::MakeSystem<Sys::TEXT>([](Game::UI::LoadingScreen const& _ls)
		{
			Core::Render::Text::Write(fVec2{ 10, 200 }, absl::StrFormat("%d/%d loaded - %s", _ls.m_currentlyLoaded, _ls.m_totalToLoad, _ls.m_nextLoadedFilename).c_str(), 10.0f);
		});

		Core::MakeSystem<Sys::RENDER_QUEUE>([](Game::UI::LoadingScreen& _loadingScreen)
		{
			if (_loadingScreen.m_fullScreenSprite.IsNull())
			{
				bool const success = Core::Resource::LoadSprite("assets/sprites/loading/loading.spr", _loadingScreen.m_fullScreenSprite);
				kaAssert(success);
			}

			fTrans2D fullScreenSpriteTrans; // default will cover the whole screen.
			Core::Render::AddSpriteToScene(_loadingScreen.m_fullScreenSprite, fullScreenSpriteTrans);
		});
	}
}