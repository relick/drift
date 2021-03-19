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
		//--------------------------------------------------------------------------------
		Core::MakeSystem<Sys::GAME>([](Core::EntityID::CoreType _entityID, Core::Sound::FadeInBGM& _fadeInBGM, Core::FrameData const& _fd)
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

		//--------------------------------------------------------------------------------
		Core::MakeSystem<Sys::GAME>([](Core::EntityID::CoreType _entityID, Core::Sound::FadeOutBGM& _fadeOutBGM, Core::FrameData const& _fd)
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

		//--------------------------------------------------------------------------------
		Core::MakeSystem<Sys::GAME>([](Core::EntityID::CoreType _entityID, Core::Sound::FadeChangeBGM& _fadeBGM, Core::FrameData const& _fd)
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

		//--------------------------------------------------------------------------------
		Core::MakeSystem<Sys::GAME>([](Core::EntityID::CoreType _entityID, Core::FrameData const& _fd, Core::Sound::SoundEffect3D const& _sfx)
		{
			if (Core::Sound::SoundEffectEnded(_sfx.m_handle))
			{
				Core::RemoveComponent<Core::Sound::SoundEffect3D>(_entityID);
			}
		});

		//--------------------------------------------------------------------------------
		Core::MakeSystem<Sys::GAME>([](Core::FrameData const& _fd, Core::Sound::SoundEffect3D& _sfx, Core::Transform3D const& _t)
		{
			fTrans const worldT = _t.CalculateWorldTransform();
			if (_sfx.m_lastPos.has_value() && _fd.dt != 0.0f)
			{
				fVec3 const vel = (worldT.m_origin - *_sfx.m_lastPos) / _fd.dt;
				Core::Sound::UpdateSoundEffect3D(_sfx.m_handle, worldT.m_origin, vel);
			}
			else
			{
				Core::Sound::UpdateSoundEffect3D(_sfx.m_handle, worldT.m_origin);
			}
			_sfx.m_lastPos = worldT.m_origin;
		});

		//--------------------------------------------------------------------------------
		Core::MakeSystem<Sys::GAME>([](Core::FrameData const& _fd, Core::Render::Camera& _cam, Core::Transform3D const& _t)
		{
			fTrans const worldT = _t.CalculateWorldTransform();
			if (_cam.m_lastPos.has_value() && _fd.dt != 0.0f)
			{
				fVec3 const vel = (worldT.m_origin - *_cam.m_lastPos) / _fd.dt;
				Core::Sound::SetHead3DParams(worldT, vel);
			}
			else
			{
				Core::Sound::SetHead3DTransform(worldT);
			}
			_cam.m_lastPos = worldT.m_origin;
		});
	}
}