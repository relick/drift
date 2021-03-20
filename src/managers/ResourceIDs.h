#pragma once

#include "ID.h"

namespace Core::Resource
{
	using TextureID = SokolIDWrapper<sg_image, true>;
	using TextureSampleID = SokolIDWrapper<sg_image, false>;

	struct ModelIDType {};
	using ModelID = ID<ModelIDType>;

	struct SpriteIDType {};
	using SpriteID = ID<SpriteIDType>;


	struct SoundEffectIDType {};
	using SoundEffectID = ID<SoundEffectIDType>;

	struct MusicIDType {};
	using MusicID = ID<MusicIDType>;


	// underlying ID == fons ID
	struct FontIDType {};
	using FontID = ID<FontIDType, int>;
}