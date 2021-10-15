#pragma once

#include "ID.h"

namespace Core::Resource
{
	using TextureID = SokolIDWrapper<sg_image, true>;
	using TextureSampleID = SokolIDWrapper<sg_image, false>;

	using ModelID = ID<struct ModelIDType>;

	using SpriteID = ID<struct SpriteIDType>;


	using SoundEffectID = ID<struct SoundEffectIDType>;

	using MusicID = ID<struct MusicIDType>;


	// underlying ID == fons ID
	using FontID = ID<struct FontIDType, int>;
}