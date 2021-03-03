#pragma once

#include "ResourceIDs.h"

namespace SoLoud
{
	typedef unsigned int handle;
}

namespace Core::Sound
{
	void Init();
	void Cleanup();

	// Sound effects
	void PlaySoundEffect(Resource::SoundEffectID _soundEffect, float _volume = -1.0f); // fire and forget
	SoLoud::handle AddSoundEffect3D(Resource::SoundEffectID _soundEffect, fVec3 const& _pos, float _volume = -1.0f); // controlled by a component
	void UpdateSoundEffect3D(SoLoud::handle _handle, fVec3 const& _pos, fVec3 const& _vel);
	void UpdateSoundEffect3D(SoLoud::handle _handle, fVec3 const& _pos);
	bool SoundEffectEnded(SoLoud::handle _handle);
	
	// BGM
	void PlayBGM(Resource::MusicID _music, float _initVolume = -1.0f); // if another BGM already playing, it is replaced, if same BGM is playing, it is unpaused
	void PauseBGM();
	void EndBGM();
	void ChangeBGMVolume(float _volume);
	float GetBGMVolume();

	// 3D Sound
	void SetHead3DParams(fTrans const& _t, fVec3 const& _vel);
	void SetHead3DTransform(fTrans const& _t);
}