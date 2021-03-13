#pragma once

#include "ID.h"

namespace Core::Resource
{
	struct TextureIDType {};
	using TextureID = ID<TextureIDType>;

	struct ModelIDType {};
	using ModelID = ID<ModelIDType>;

	struct SpriteIDType {};
	using SpriteID = ID<SpriteIDType>;


	struct SoundEffectIDType {};
	using SoundEffectID = ID<SoundEffectIDType>;

	struct MusicIDType {};
	using MusicID = ID<MusicIDType>;

}