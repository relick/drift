#include "UISystems.h"

#include "components.h"
#include "systems.h"
#include "SystemOrdering.h"

#include "managers/EntityManager.h"

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
				Core::RemoveComponent<Game::UI::LoadingScreen>(_entity);
				// fixme
				// Core::DestroyEntity(_entity);
				// temp
				Core::Scene::AddDemoScene();
				Core::RemoveComponent<Core::Render::Sprite>(_entity);
				return;
			}
			if (preload->m_currentLoadingIndex < preload->m_filesToLoad.size())
			{
				_loadingScreen.m_nextLoadedFilename = preload->m_filesToLoad[preload->m_currentLoadingIndex].m_filePath;
			}
		});

		Core::MakeSystem<Sys::TEXT>([](Game::UI::LoadingScreen const& _loadingScreen)
		{
			Core::Render::Text::Write(fVec2{ 10, 200 }, absl::StrFormat("Loading %s", _loadingScreen.m_nextLoadedFilename).c_str());
		});
	}
}