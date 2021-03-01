#pragma once

#include "ResourceIDs.h"

namespace Core::Sound
{
	void Init();
	void Cleanup();

	// Sound effects
	void PlaySoundEffect(Resource::SoundEffectID _soundEffect);
	void PlaySoundEffect3D(Resource::SoundEffectID _soundEffect, fVec3 const& _location);
	
	// BGM
	void PlayBGM(Resource::MusicID _music, float _initVolume = -1.0f); // if another BGM already playing, it is replaced, if same BGM is playing, it is unpaused
	void PauseBGM();
	void EndBGM();
	void ChangeBGMVolume(float _volume);
	float GetBGMVolume();

	// 3D Music
}