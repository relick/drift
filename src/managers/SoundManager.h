#pragma once

#include "ID.h"

namespace Core::Sound
{
	struct SoundEffectIDType {};
	using SoundEffectID = ID<SoundEffectIDType>;

	struct MusicIDType {};
	using MusicID = ID<MusicIDType>;

	void Init();
	void Cleanup();

	// Sound effects
	void PlaySoundEffect(SoundEffectID _soundEffect);
	void PlaySoundEffect3D(SoundEffectID _soundEffect, fVec3 const& _location);
	
	// BGM
	void StartBGM(MusicID _music); // if BGM already playing, ends that first and plays new music
	void EndBGM();
	void ChangeBGMVolume(float _volume);

	// 3D Music
}