#pragma once

#include "common.h"
#include "managers/EntityManager.h"
#include "managers/ResourceIDs.h"

#include <ecs/flags.h>

namespace Core::Sound
{
	struct FadeInBGM
	{

	};

	struct FadeOutBGM
	{

	};

	struct CrossfadeBGM
	{

	};

	struct BGMDesc
	{
		std::string m_filePath;
		float m_initVolume{ -1.0f };
	};

	struct BGM
	{
		use_initialiser;

		Resource::MusicID m_music;
	};

}

namespace Core
{
	template<>
	void AddComponent(EntityID const _entity, Sound::BGMDesc const& _desc);

	template<>
	void RemoveComponent<Sound::BGM>(EntityID const _entity);
}