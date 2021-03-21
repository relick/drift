#pragma once

#include "common.h"
#include "managers/EntityManager.h"
#include "managers/ResourceIDs.h"

#include <ecs/flags.h>

namespace SoLoud
{
	typedef unsigned int handle;
}

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

	struct SoundEffect3DDesc
	{
		std::string m_filePath;
		float m_initVolume{ -1.0f };
	};

	struct SoundEffect3D
	{
		use_initialiser;

		Resource::SoundEffectID m_soundEffect;
		std::optional<fVec3> m_lastPos{};
		SoLoud::handle m_handle{ 0 };
	};

}

namespace Core
{
	template<>
	void AddComponent(EntityID const _entity, Sound::BGMDesc const& _desc);

	template<>
	void CleanupComponent<Sound::BGM>(EntityID const _entity);

	template<>
	void AddComponent(EntityID const _entity, Sound::SoundEffect3DDesc const& _desc);
}