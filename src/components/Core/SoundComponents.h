#pragma once

#include "common.h"
#include "managers/EntityManager.h"
#include "managers/ResourceIDs.h"

#include <ecs/flags.h>

namespace SoLoud
{
	// FIXME: strong typedef
	using handle = uint32;
}

namespace Core::Sound
{
	struct FadeInBGM
	{
		Vec1 m_targetVolume{ 1.0f };
		Vec1 m_timeToFadeIn{ 1.0f };

		Vec1 m_timePassed{ 0.0f };
	};

	struct FadeOutBGM
	{
		Vec1 m_timeToFadeOut{ 1.0f };
		Vec1 m_startVolume{ -1.0f };

		Vec1 m_timePassed{ 0.0f };
	};

	struct FadeChangeBGM
	{
		Vec1 m_timeToFade{ 1.0f };
		Vec1 m_startVolume{ -1.0f };
		Vec1 m_targetVolume{ 1.0f };
		std::string m_nextBGMFilePath;

		bool m_fadingOut{ true };
		Vec1 m_timePassed{ 0.0f };
	};

	struct BGMDesc
	{
		std::string m_filePath;
		Vec1 m_initVolume{ -1.0f };
	};

	struct BGM
	{
		use_initialiser;

		Resource::MusicID m_music;
	};

	struct SoundEffect3DDesc
	{
		std::string m_filePath;
		Vec1 m_initVolume{ -1.0f };
	};

	struct SoundEffect3D
	{
		use_initialiser;

		Resource::SoundEffectID m_soundEffect;
		std::optional<Vec3> m_lastPos{};
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