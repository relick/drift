#include "common.h"

#include "SoundComponents.h"
#include "components.h"
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
			Core::ECS::AddComponent(_entity, newComponent);
			Sound::PlayBGM(musicID, _desc.m_initVolume);
		}
	}

	template<>
	void RemoveComponent<Sound::BGM>(EntityID const _entity)
	{
		Sound::EndBGM();
		Core::ECS::RemoveComponent<Sound::BGM>(_entity);
	}

	template<>
	void AddComponent(EntityID const _entity, Sound::SoundEffect3DDesc const& _desc)
	{
		Sound::SoundEffect3D newComponent{};

		Resource::SoundEffectID const sfxID = Resource::GetSoundEffectID(_desc.m_filePath);

		ASSERT(sfxID.IsValid(), "couldn't load music, not adding component");
		if (sfxID.IsValid())
		{
			// Add to ecs
			Core::Transform const* transform = Core::GetComponent<Core::Transform>(_entity);
			ASSERT(transform != nullptr, "missing Transform component when trying to add SoundEffect3D");

			newComponent.m_soundEffect = sfxID;
			newComponent.m_handle = Sound::AddSoundEffect3D(sfxID, transform->CalculateWorldTransform().m_origin);

			Core::ECS::AddComponent(_entity, newComponent);
		}
	}
}