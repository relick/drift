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
			ecs::add_component(_entity.GetValue(), newComponent);
			Sound::PlayBGM(musicID, _desc.m_initVolume);
		}
	}

	template<>
	void RemoveComponent<Sound::BGM>(EntityID const _entity)
	{
		Sound::BGM* const oldComponent = ecs::get_component<Sound::BGM>(_entity.GetValue());
		ASSERT(oldComponent);

		Sound::EndBGM();

		ecs::remove_component<Sound::BGM>(_entity.GetValue());
	}
}