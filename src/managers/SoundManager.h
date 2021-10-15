#pragma once

#include "ResourceIDs.h"

namespace SoLoud
{
	// FIXME: strong typedef
	using handle = unsigned int;
}

namespace Core::Sound
{
	void Init();
	void Cleanup();

	// Sound effects
	void PlaySoundEffect(Resource::SoundEffectID _soundEffect, Vec1 _volume = -1.0f); // fire and forget
	SoLoud::handle AddSoundEffect3D(Resource::SoundEffectID _soundEffect, Vec3 const& _pos, Vec1 _volume = -1.0f); // controlled by a component
	void UpdateSoundEffect3D(SoLoud::handle _handle, Vec3 const& _pos, Vec3 const& _vel);
	void UpdateSoundEffect3D(SoLoud::handle _handle, Vec3 const& _pos);
	bool SoundEffectEnded(SoLoud::handle _handle);
	
	// BGM
	void PlayBGM(Resource::MusicID _music, Vec1 _initVolume = -1.0f); // if another BGM already playing, it is replaced, if same BGM is playing, it is unpaused
	void PauseBGM();
	void EndBGM();
	void ChangeBGMVolume( Vec1 _volume);
	Vec1 GetBGMVolume();

	// 3D Sound
	void SetHead3DParams(Trans const& _t, Vec3 const& _vel);
	void SetHead3DTransform(Trans const& _t);
}