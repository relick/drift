#pragma once

#include "common.h"
#include "managers/ResourceIDs.h"
#include "managers/EntityManager.h"

#include <string>
#include <memory>

namespace Game::UI
{
	struct LoadingScreen
	{
		std::string m_nextLoadedFilename{ "assets/preload.res" };
		usize m_currentlyLoaded{ 0 };
		usize m_totalToLoad{ 0 };
		Core::Resource::SpriteID m_fullScreenSprite;

		std::shared_ptr< Core::Scene::BaseScene > m_nextScene;
	};
}

namespace Core
{
	template<>
	void AddComponent(EntityID const _entity, Game::UI::LoadingScreen const& _component);
}