#include "common.h"

#include "SoundManager.h"
#include "managers/ResourceManager.h"

#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

namespace Core::Sound
{
	struct CurrentlyPlayingBGM
	{
		Resource::MusicID music;
		SoLoud::handle handle;
	};

	struct SoundState
	{
		SoLoud::Soloud soloud;
		CurrentlyPlayingBGM currentPlayingBGM;
	};

	static SoundState g_soundState;

	//--------------------------------------------------------------------------------
	void Init()
	{
		g_soundState.soloud.init();
	}

	//--------------------------------------------------------------------------------
	void Cleanup()
	{
		g_soundState.soloud.deinit();
	}

	//--------------------------------------------------------------------------------
	void PlaySoundEffect
	(
		Resource::SoundEffectID _soundEffect,
		Vec1 _volume // = -1.0f
	)
	{
		Resource::SoundEffectData& sfxData = Resource::GetSoundEffect(_soundEffect);
		g_soundState.soloud.play(sfxData.m_sound, _volume);
	}

	//--------------------------------------------------------------------------------
	SoLoud::handle AddSoundEffect3D
	(
		Resource::SoundEffectID _soundEffect,
		Vec3 const& _pos,
		Vec1 _volume // = -1.0f
	)
	{
		Resource::SoundEffectData& sfxData = Resource::GetSoundEffect(_soundEffect);
		return g_soundState.soloud.play3d(sfxData.m_sound,
			_pos.x, _pos.y, _pos.z,
			0.0f, 0.0f, 0.0f,
			_volume);
	}

	//--------------------------------------------------------------------------------
	void UpdateSoundEffect3D
	(
		SoLoud::handle _handle,
		Vec3 const& _pos,
		Vec3 const& _vel
	)
	{
		g_soundState.soloud.set3dSourceParameters(_handle,
			_pos.x, _pos.y, _pos.z,
			_vel.x, _vel.y, _vel.z
		);
	}

	//--------------------------------------------------------------------------------
	void UpdateSoundEffect3D
	(
		SoLoud::handle _handle,
		Vec3 const& _pos
	)
	{
		g_soundState.soloud.set3dSourcePosition(_handle,
			_pos.x, _pos.y, _pos.z
		);
	}

	//--------------------------------------------------------------------------------
	bool SoundEffectEnded
	(
		SoLoud::handle _handle
	)
	{
		return g_soundState.soloud.isValidVoiceHandle(_handle);
	}

	//--------------------------------------------------------------------------------
	void PlayBGM
	(
		Resource::MusicID _music,
		Vec1 _initVolume // = -1.0f
	)
	{
		if (g_soundState.currentPlayingBGM.music.IsValid())
		{
			if (g_soundState.currentPlayingBGM.music != _music)
			{
				g_soundState.soloud.stop(g_soundState.currentPlayingBGM.handle);
			}
			else
			{
				g_soundState.soloud.setPause(g_soundState.currentPlayingBGM.handle, false);
				return;
			}
		}
		Resource::MusicData& musicData = Resource::GetMusic(_music);
		g_soundState.currentPlayingBGM.handle = g_soundState.soloud.playBackground(musicData.m_music, _initVolume);
		g_soundState.currentPlayingBGM.music = _music;
	}

	//--------------------------------------------------------------------------------
	void PauseBGM()
	{
		if (g_soundState.currentPlayingBGM.music.IsValid())
		{
			g_soundState.soloud.setPause(g_soundState.currentPlayingBGM.handle, true);
		}
	}

	//--------------------------------------------------------------------------------
	void EndBGM()
	{
		if (g_soundState.currentPlayingBGM.music.IsValid())
		{
			g_soundState.soloud.stop(g_soundState.currentPlayingBGM.handle);
			g_soundState.currentPlayingBGM.handle = 0;
			g_soundState.currentPlayingBGM.music = Resource::MusicID();
		}
	}

	//--------------------------------------------------------------------------------
	void ChangeBGMVolume
	(
		Vec1 _volume
	)
	{
		if (g_soundState.currentPlayingBGM.music.IsValid())
		{
			g_soundState.soloud.setVolume(g_soundState.currentPlayingBGM.handle, _volume);
		}
	}

	//--------------------------------------------------------------------------------
	Vec1 GetBGMVolume()
	{
		if (g_soundState.currentPlayingBGM.music.IsValid())
		{
			return g_soundState.soloud.getVolume(g_soundState.currentPlayingBGM.handle);
		}
		return 0.0f;
	}

	//--------------------------------------------------------------------------------
	void SetHead3DParams
	(
		Trans const& _t,
		Vec3 const& _vel
	)
	{
		g_soundState.soloud.set3dListenerParameters(
			_t.m_origin.x, _t.m_origin.y, _t.m_origin.z,
			_t.Forward().x, _t.Forward().y, _t.Forward().z,
			_t.Up().x, _t.Up().y, _t.Up().z,
			_vel.x, _vel.y, _vel.z
		);
	}

	//--------------------------------------------------------------------------------
	void SetHead3DTransform
	(
		Trans const& _t
	)
	{
		g_soundState.soloud.set3dListenerPosition(
			_t.m_origin.x, _t.m_origin.y, _t.m_origin.z
		);
		g_soundState.soloud.set3dListenerAt(
			_t.Forward().x, _t.Forward().y, _t.Forward().z
		);
		g_soundState.soloud.set3dListenerUp(
			_t.Up().x, _t.Up().y, _t.Up().z
		);
	}
}