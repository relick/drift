#pragma once

#include "common.h"
#include "ResourceIDs.h"

#include <vector>
#include <string>
#include <optional>
#include <sokol_gfx.h>

#include <soloud_wav.h>
#include <soloud_wavstream.h>

#include "shaders/main.h"

struct aiNode;
struct aiScene;
struct aiMesh;

namespace Core
{
	namespace Resource
	{
		void Init();
		void Setup();
		void Cleanup();

		struct TextureData
		{
			enum class Type
			{
				Diffuse,
				Specular,
				Normal,
			};

			sg_image m_texID;
			Type m_type;
			std::string m_path;
		};

		struct VertexData
		{
			fVec3 position;
			fVec3 normal;
			fVec2 uv{ 0.0f, 0.0f };
		};

		using IndexType = uint32;
		using MaterialData = main_material_t;
		struct MeshLoadData
		{
			std::vector<VertexData> m_vertices;
			std::vector<IndexType> m_indices;
		};

		struct MeshData
		{
			std::optional<MeshLoadData> m_loadData;

			uint32 m_indexCount{ 0 };
			MaterialData m_material;
			std::vector<TextureID> m_textures;
			sg_bindings m_bindings{};

			uint32 NumToDraw() const { return m_indexCount; }
			void CleanData()
			{
				if (m_loadData.has_value())
				{
					m_indexCount = static_cast<uint32>(m_loadData->m_indices.size());
					m_loadData = std::optional<MeshLoadData>{};
				}
			}
		};
		
		struct ModelLoadData
		{
			std::vector<float> m_vertexBufferData;
			std::vector<uint32> m_indexBufferData;
		};

		struct ModelData
		{
			std::optional<ModelLoadData> m_loadData;
			// for now just stores meshes, no transform tree
			std::vector<MeshData> m_meshes;
			std::string m_path;

			void CleanData()
			{
				if (m_loadData.has_value())
				{
					m_loadData = std::optional<ModelLoadData>{};
				}
			}
#if DEBUG_TOOLS
			std::string _traceName_vBufData;
			std::string _traceName_iBufData;
#endif
		};

		TextureData const& GetTexture(TextureID _texture);
		ModelData const& GetModel(ModelID _model);

		bool LoadModel(std::string const& _path, ModelID& o_modelID);

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

		SoundEffectID GetSoundEffectID(std::string const& _path);
		MusicID GetMusicID(std::string const& _path);
		SoundEffectData& GetSoundEffect(SoundEffectID _soundEffect);
		MusicData& GetMusic(MusicID _music);
	}
}