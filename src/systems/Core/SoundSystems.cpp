#include "common.h"

#include "SoundSystems.h"
#include "components.h"
#include "SystemOrdering.h"
#include "managers/EntityManager.h"
#include "managers/SoundManager.h"

namespace Core::Sound
{
	void Setup()
	{
		ecs::make_system<ecs::opts::group<Sys::GAME>>([](ecs::entity_id _entityID, Core::Sound::FadeInBGM& _fadeInBGM, Core::FrameData const& _fd)
		{
			_fadeInBGM.m_timePassed += _fd.unscaled_dt;
			if (_fadeInBGM.m_timePassed >= _fadeInBGM.m_timeToFadeIn)
			{
				Core::Sound::ChangeBGMVolume(_fadeInBGM.m_targetVolume);
				Core::RemoveComponent<Core::Sound::FadeInBGM>(_entityID);
			}
			else
			{
				Core::Sound::ChangeBGMVolume(_fadeInBGM.m_targetVolume * (_fadeInBGM.m_timePassed / _fadeInBGM.m_timeToFadeIn));
			}
		});

		ecs::make_system<ecs::opts::group<Sys::GAME>>([](ecs::entity_id _entityID, Core::Sound::FadeOutBGM& _fadeOutBGM, Core::FrameData const& _fd)
		{
			if (_fadeOutBGM.m_startVolume < 0.0f)
			{
				_fadeOutBGM.m_startVolume = Core::Sound::GetBGMVolume();
			}

			_fadeOutBGM.m_timePassed += _fd.unscaled_dt;
			if (_fadeOutBGM.m_timePassed >= _fadeOutBGM.m_timeToFadeOut)
			{
				Core::Sound::EndBGM();
				Core::RemoveComponent<Core::Sound::FadeOutBGM>(_entityID);
				Core::RemoveComponent<Core::Sound::BGM>(_entityID);
			}
			else
			{
				Core::Sound::ChangeBGMVolume(_fadeOutBGM.m_startVolume * (1.0f - (_fadeOutBGM.m_timePassed / _fadeOutBGM.m_timeToFadeOut)));
			}
		});

		ecs::make_system<ecs::opts::group<Sys::GAME>>([](ecs::entity_id _entityID, Core::Sound::FadeChangeBGM& _fadeBGM, Core::FrameData const& _fd)
		{
			if (_fadeBGM.m_startVolume < 0.0f)
			{
				_fadeBGM.m_startVolume = Core::Sound::GetBGMVolume();
			}

			_fadeBGM.m_timePassed += _fd.unscaled_dt;
			if (_fadeBGM.m_timePassed >= _fadeBGM.m_timeToFade)
			{
				if (_fadeBGM.m_fadingOut)
				{
					Core::Sound::EndBGM();
					Core::RemoveComponent<Core::Sound::BGM>(_entityID);
					Core::AddComponent(_entityID, Core::Sound::BGMDesc{ .m_filePath = _fadeBGM.m_nextBGMFilePath, .m_initVolume = 0.0f, });
					_fadeBGM.m_fadingOut = false;
					_fadeBGM.m_timePassed = 0.0f;
				}
				else
				{
					Core::RemoveComponent<Core::Sound::FadeChangeBGM>(_entityID);
				}
			}
			else
			{
				if (_fadeBGM.m_fadingOut)
				{
					Core::Sound::ChangeBGMVolume(_fadeBGM.m_startVolume * (1.0f - (_fadeBGM.m_timePassed / _fadeBGM.m_timeToFade)));
				}
				else
				{
					Core::Sound::ChangeBGMVolume(_fadeBGM.m_targetVolume * (_fadeBGM.m_timePassed / _fadeBGM.m_timeToFade));
				}
			}
		});
	}
}