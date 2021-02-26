#include "common.h"

#include "Sound.h"

#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>
#include "..\systems\Core\Sound.h"

namespace Core::Sound
{
	struct
	{
		SoLoud::Soloud soloud;
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
		SoundEffectID _soundEffect
	)
	{

	}

	//--------------------------------------------------------------------------------
	void PlaySoundEffect3D
	(
		SoundEffectID _soundEffect,
		fVec3 const& _location
	)
	{

	}

	//--------------------------------------------------------------------------------
	void StartBGM
	(
		MusicID _music
	)
	{

	}

	//--------------------------------------------------------------------------------
	void EndBGM()
	{

	}

	//--------------------------------------------------------------------------------
	void ChangeBGMVolume
	(
		float _volume
	)
	{

	}
}