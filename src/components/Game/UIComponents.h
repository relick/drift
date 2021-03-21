#pragma once

#include "common.h"
#include "managers/ResourceIDs.h"

#include <string>

namespace Game::UI
{
	struct LoadingScreen
	{
		std::string m_nextLoadedFilename{ "assets/preload.res" };
		usize m_currentlyLoaded{ 0 };
		usize m_totalToLoad{ 0 };
		Core::Resource::SpriteID m_fullScreenSprite;
	};

}