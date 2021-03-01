#include "common.h"

#include "SoundComponents.h"
#include "managers/ResourceManager.h"
#include "managers/SoundManager.h"

namespace Core
{
	template<>
	void AddComponent(EntityID const _entity, Sound::BGMDesc const& _desc)
	{
		Sound::BGM newComponent{};

		Resource::MusicID const musicID = Resource::GetMusicID(_desc.m_filePath);

		ASSERT(musicID.IsValid(), "couldn't load music, not adding component");
		if (musicID.IsValid())
		{
			// Add to ecs
			newComponent.m_music = musicID;
			ecs::add_component(_entity.GetValue(), newComponent);
			Sound::PlayBGM(musicID, _desc.m_initVolume);
		}
	}

	template<>
	void RemoveComponent<Sound::BGM>(EntityID const _entity)
	{
		Sound::EndBGM();
		ecs::remove_component<Sound::BGM>(_entity.GetValue());
	}

	template<>
	void AddComponent(EntityID const _entity, Sound::SoundEffectDesc const& _desc)
	{
		Sound::SoundEffect newComponent{};

		Resource::SoundEffectID const sfxID = Resource::GetSoundEffectID(_desc.m_filePath);

		ASSERT(sfxID.IsValid(), "couldn't load music, not adding component");
		if (sfxID.IsValid())
		{
			// Add to ecs
			newComponent.m_is3D = _desc.m_is3D;
			newComponent.m_soundEffect = sfxID;
			ecs::add_component(_entity.GetValue(), newComponent);
			if (_desc.m_is3D)
			{
				Sound::PlaySoundEffect3D(sfxID, fVec3{});
			}
			else
			{
				Sound::PlaySoundEffect(sfxID);
			}
		}
	}
}