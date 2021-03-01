#pragma once

#include "common.h"
#include "managers/EntityManager.h"
#include "managers/ResourceIDs.h"

#include <ecs/flags.h>

namespace Core::Sound
{
	struct FadeInBGM
	{
		float m_targetVolume{ 1.0f };
		float m_timeToFadeIn{ 1.0f };

		float m_timePassed{ 0.0f };
	};

	struct FadeOutBGM
	{
		float m_timeToFadeOut{ 1.0f };
		float m_startVolume{ -1.0f };

		float m_timePassed{ 0.0f };
	};

	struct FadeChangeBGM
	{
		float m_timeToFade{ 1.0f };
		float m_startVolume{ -1.0f };
		float m_targetVolume{ 1.0f };
		std::string m_nextBGMFilePath;

		bool m_fadingOut{ true };
		float m_timePassed{ 0.0f };
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

	struct SoundEffectDesc
	{
		std::string m_filePath;
		float m_initVolume{ -1.0f };
		bool m_is3D{ false };
	};

	struct SoundEffect
	{
		use_initialiser;

		Resource::SoundEffectID m_soundEffect;
		bool m_is3D;
	};

}

namespace Core
{
	template<>
	void AddComponent(EntityID const _entity, Sound::BGMDesc const& _desc);

	template<>
	void RemoveComponent<Sound::BGM>(EntityID const _entity);

	template<>
	void AddComponent(EntityID const _entity, Sound::SoundEffectDesc const& _desc);

	template<>
	void RemoveComponent<Sound::SoundEffect>(EntityID const _entity);
}