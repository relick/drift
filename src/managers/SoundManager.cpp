#include "common.h"

#include "SoundManager.h"
#include "managers/ResourceManager.h"

#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

namespace Core::Sound
{
	struct
	{
		SoLoud::Soloud soloud;
		struct
		{
			Resource::MusicID music;
			SoLoud::handle handle;
		} currentPlayingBGM;
	} soundState;

	//--------------------------------------------------------------------------------
	void Init()
	{
		soundState.soloud.init();
	}

	//--------------------------------------------------------------------------------
	void Cleanup()
	{
		soundState.soloud.deinit();
	}

	//--------------------------------------------------------------------------------
	void PlaySoundEffect
	(
		Resource::SoundEffectID _soundEffect,
		float _volume // = -1.0f
	)
	{
		Resource::SoundEffectData& sfxData = Resource::GetSoundEffect(_soundEffect);
		soundState.soloud.play(sfxData.m_sound, _volume);
	}

	//--------------------------------------------------------------------------------
	SoLoud::handle AddSoundEffect3D
	(
		Resource::SoundEffectID _soundEffect,
		fVec3 const& _pos,
		float _volume // = -1.0f
	)
	{
		Resource::SoundEffectData& sfxData = Resource::GetSoundEffect(_soundEffect);
		return soundState.soloud.play3d(sfxData.m_sound,
			_pos.x, _pos.y, _pos.z,
			0.0f, 0.0f, 0.0f,
			_volume);
	}

	//--------------------------------------------------------------------------------
	void UpdateSoundEffect3D
	(
		SoLoud::handle _handle,
		fVec3 const& _pos,
		fVec3 const& _vel
	)
	{
		soundState.soloud.set3dSourceParameters(_handle,
			_pos.x, _pos.y, _pos.z,
			_vel.x, _vel.y, _vel.z
		);
	}

	//--------------------------------------------------------------------------------
	void UpdateSoundEffect3D
	(
		SoLoud::handle _handle,
		fVec3 const& _pos
	)
	{
		soundState.soloud.set3dSourcePosition(_handle,
			_pos.x, _pos.y, _pos.z
		);
	}

	//--------------------------------------------------------------------------------
	bool SoundEffectEnded
	(
		SoLoud::handle _handle
	)
	{
		return soundState.soloud.isValidVoiceHandle(_handle);
	}

	//--------------------------------------------------------------------------------
	void PlayBGM
	(
		Resource::MusicID _music,
		float _initVolume // = -1.0f
	)
	{
		if (soundState.currentPlayingBGM.music.IsValid())
		{
			if (soundState.currentPlayingBGM.music != _music)
			{
				soundState.soloud.stop(soundState.currentPlayingBGM.handle);
			}
			else
			{
				soundState.soloud.setPause(soundState.currentPlayingBGM.handle, false);
				return;
			}
		}
		Resource::MusicData& musicData = Resource::GetMusic(_music);
		soundState.currentPlayingBGM.handle = soundState.soloud.playBackground(musicData.m_music, _initVolume);
		soundState.currentPlayingBGM.music = _music;
	}

	//--------------------------------------------------------------------------------
	void PauseBGM()
	{
		if (soundState.currentPlayingBGM.music.IsValid())
		{
			soundState.soloud.setPause(soundState.currentPlayingBGM.handle, true);
		}
	}

	//--------------------------------------------------------------------------------
	void EndBGM()
	{
		if (soundState.currentPlayingBGM.music.IsValid())
		{
			soundState.soloud.stop(soundState.currentPlayingBGM.handle);
			soundState.currentPlayingBGM.handle = 0;
			soundState.currentPlayingBGM.music = Resource::MusicID();
		}
	}

	//--------------------------------------------------------------------------------
	void ChangeBGMVolume
	(
		float _volume
	)
	{
		if (soundState.currentPlayingBGM.music.IsValid())
		{
			soundState.soloud.setVolume(soundState.currentPlayingBGM.handle, _volume);
		}
	}

	//--------------------------------------------------------------------------------
	float GetBGMVolume()
	{
		if (soundState.currentPlayingBGM.music.IsValid())
		{
			return soundState.soloud.getVolume(soundState.currentPlayingBGM.handle);
		}
		return 0.0f;
	}

	//--------------------------------------------------------------------------------
	void SetHead3DParams
	(
		fTrans const& _t,
		fVec3 const& _vel
	)
	{
		soundState.soloud.set3dListenerParameters(
			_t.m_origin.x, _t.m_origin.y, _t.m_origin.z,
			_t.forward().x, _t.forward().y, _t.forward().z,
			_t.up().x, _t.up().y, _t.up().z,
			_vel.x, _vel.y, _vel.z
		);
	}

	//--------------------------------------------------------------------------------
	void SetHead3DTransform
	(
		fTrans const& _t
	)
	{
		soundState.soloud.set3dListenerPosition(
			_t.m_origin.x, _t.m_origin.y, _t.m_origin.z
		);
		soundState.soloud.set3dListenerAt(
			_t.forward().x, _t.forward().y, _t.forward().z
		);
		soundState.soloud.set3dListenerUp(
			_t.up().x, _t.up().y, _t.up().z
		);
	}
}