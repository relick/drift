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
		Resource::SoundEffectID _soundEffect
	)
	{

	}

	//--------------------------------------------------------------------------------
	void PlaySoundEffect3D
	(
		Resource::SoundEffectID _soundEffect,
		fVec3 const& _location
	)
	{

	}

	//--------------------------------------------------------------------------------
	void PlayBGM
	(
		Resource::MusicID _music,
		float _initVolume
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
}