#pragma once

#include "common.h"
#include "managers/RenderIDs.h"
#include "managers/EntityManager.h"

#include <string>
#include <memory>

namespace Game::UI
{
	struct LoadingScreen
	{
		use_initialiser;

		std::string m_nextLoadedFilename;
		usize m_currentlyLoaded{ 0 };
		usize m_totalToLoad{ 0 };
		Core::Render::SpriteSceneID m_fullScreenSprite;

		std::shared_ptr< Core::Scene::BaseScene > m_nextScene;
	};

	struct SceneLoadDesc
	{
		std::shared_ptr< Core::Scene::BaseScene > m_nextScene;
	};
}

namespace Core
{
	template<>
	void AddComponent(EntityID const _entity, Game::UI::SceneLoadDesc const& _component);

	template<>
	void CleanupComponent<Game::UI::LoadingScreen>( EntityID const _entity );
}