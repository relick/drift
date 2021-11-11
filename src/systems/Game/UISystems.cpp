#include "UISystems.h"

#include "components.h"
#include "systems.h"

#include "managers/EntityManager.h"
#include "managers/RenderManager.h"
#include "managers/ResourceManager.h"
#include "managers/TextManager.h"

#include "scenes/CubeTest.h"

#include <format>

namespace Game::UI
{
	void Setup()
	{
		Core::MakeSystem<Sys::UI_UPDATE>([](Core::EntityID::CoreType _entity, Game::UI::LoadingScreen& _loadingScreen)
		{
			auto const preload = Core::GetComponent<Core::Resource::Preload>(_entity);
			if (preload == nullptr)
			{
				if ( _loadingScreen.m_nextScene )
				{
					Core::Scene::NextScene( _loadingScreen.m_nextScene );
				}
				else
				{
					Core::Scene::NextScene<Game::Scene::CubeTestScene>();
				}
				return;
			}
			if (preload->m_currentLoadingIndex < preload->m_filesToLoad.size())
			{
				_loadingScreen.m_nextLoadedFilename = preload->m_filesToLoad[preload->m_currentLoadingIndex].m_filePath;
			}
			_loadingScreen.m_currentlyLoaded = preload->m_currentLoadingIndex;
			_loadingScreen.m_totalToLoad = preload->m_filesToLoad.size();
		});

		Core::MakeSystem<Sys::TEXT>([](Core::MT_Only&, Game::UI::LoadingScreen const& _ls)
		{
			Core::Render::Text::Write(Vec2{ 10, 200 }, std::format("{:d}/{:d} loaded - {:s}", _ls.m_currentlyLoaded, _ls.m_totalToLoad, _ls.m_nextLoadedFilename).c_str(), 10.0f);
		});

		Core::MakeSystem<Sys::RENDER_QUEUE>([](Game::UI::LoadingScreen& _loadingScreen)
		{
			Trans2D fullScreenSpriteTrans; // default will cover the whole screen.
			Core::Render::AddSpriteToScene(_loadingScreen.m_fullScreenSprite, fullScreenSpriteTrans);
		});
	}
}