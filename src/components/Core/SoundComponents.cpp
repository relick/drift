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

		bool const loaded = Resource::LoadMusic(_desc.m_filePath, newComponent.m_music);

		kaAssert(loaded, "couldn't load music, not adding component");
		if (loaded)
		{
			// Add to ecs
			Core::ECS::AddComponent(_entity, newComponent);
			Sound::PlayBGM(newComponent.m_music, _desc.m_initVolume);
		}
	}

	template<>
	void CleanupComponent<Sound::BGM>(EntityID const _entity)
	{
		Sound::EndBGM();
	}

	template<>
	void AddComponent(EntityID const _entity, Sound::SoundEffect3DDesc const& _desc)
	{
		Sound::SoundEffect3D newComponent{};

		bool const loaded = Resource::LoadSoundEffect(_desc.m_filePath, newComponent.m_soundEffect);

		kaAssert(loaded, "couldn't load music, not adding component");
		if (loaded)
		{
			// Add to ecs
			Core::Transform3D const* transform = Core::GetComponent<Core::Transform3D>(_entity);
			kaAssert(transform != nullptr, "missing Transform component when trying to add SoundEffect3D");

			newComponent.m_handle = Sound::AddSoundEffect3D(newComponent.m_soundEffect, transform->CalculateWorldTransform().m_origin);

			Core::ECS::AddComponent(_entity, newComponent);
		}
	}
}