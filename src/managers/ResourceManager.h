#pragma once

#include "common.h"
#include "ResourceIDs.h"

#include <vector>
#include <string>
#include <sokol_gfx.h>

#include <soloud_wav.h>
#include <soloud_wavstream.h>

#include "shaders/main.h"

struct aiNode;
struct aiScene;
struct aiMesh;

// structs
namespace Core
{
	namespace Resource
	{
		//-------------------------------------------------
		struct TextureData
		{
			enum class Type
			{
				Diffuse,
				Specular,
				Normal,
				Cubemap,
			};

			sg_image m_texID;
			Type m_type;
			std::string m_path;
		};

		//-------------------------------------------------
		struct VertexData
		{
			fVec3 position;
			fVec3 normal;
			fVec2 uv{ 0.0f, 0.0f };
			fVec3 tangent;
		};
		using IndexType = uint32;

		//-------------------------------------------------
		using MaterialData = main_material_t;

		struct MeshData
		{
			uint32 m_indexCount{ 0 };
			MaterialData m_material;
			std::vector<TextureID> m_textures;
			sg_bindings m_bindings{};

			uint32 NumToDraw() const { return m_indexCount; }
			void SetNumToDraw(uint32 _indexCount) { m_indexCount = _indexCount; }
		};

		//-------------------------------------------------
		struct ModelData
		{
			// for now just stores meshes, no transform tree
			std::vector<MeshData> m_meshes;
			std::string m_path;

#if DEBUG_TOOLS
			std::string _traceName_vBufData;
			std::string _traceName_iBufData;
#endif
		};

		//-------------------------------------------------
		struct SpriteData
		{
			std::string m_path;
			struct
			{
				TextureID id;
				sg_image image;
			} m_texture;
			fVec2 m_dimensions;
			fVec2 m_topLeftUV;
		};

		//-------------------------------------------------
		struct SoundEffectData
		{
			std::string m_path;
			SoLoud::Wav m_sound;
		};

		struct MusicData
		{
			std::string m_path;
			SoLoud::WavStream m_music;
		};
	}
}

// functions
namespace Core::Resource
{
	void Init();
	void Setup();
	void Cleanup();

	TextureData const& GetTexture(TextureID _texture);
	ModelData const& GetModel(ModelID _model);
	SpriteData const& GetSprite(SpriteID _sprite);
	SoundEffectData& GetSoundEffect(SoundEffectID _soundEffect);
	MusicData& GetMusic(MusicID _music);

	bool LoadModel(std::string const& _path, ModelID& o_modelID);
	bool LoadCubemap(std::string const& _folderPath, TextureID& o_cubemapID);
	bool LoadSprite(std::string const& _path, SpriteID& o_spriteID);
	bool LoadSoundEffect(std::string const& _path, SoundEffectID& o_soundEffectID);
	bool LoadMusic(std::string const& _path, MusicID& o_musicID);
}