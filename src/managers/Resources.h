#pragma once

#include "common.h"
#include "ID.h"

#include <vector>
#include <string>
#include <sokol_gfx.h>

struct aiNode;
struct aiScene;
struct aiMesh;

#define USE_INTERLEAVED (1)

namespace Core
{
	namespace Resource
	{
		void Init();

		struct TextureIDType {};
		using TextureID = ID<TextureIDType>;

		struct ModelIDType {};
		using ModelID = ID<ModelIDType>;

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

		struct MaterialData
		{
			fVec3Data m_diffuseColour;
			fVec3Data m_specularColour;
			fVec3Data m_ambientColour;
			float m_shininess;

			std::vector<TextureID> m_textures;
		};

		struct VertexData
		{
			fVec3Data position;
			fVec3Data normal;
			fVec2Data uv{ 0.0f, 0.0f };
		};

		using index_type = uint32;

		struct MeshData
		{
			std::vector<VertexData> m_vertices;
			sg_bindings m_bindings{};

			std::vector<index_type> m_indices;
			MaterialData m_material;
		};
		
		struct ModelData
		{
			// for now just stores meshes, no transform tree
			std::vector<MeshData> m_meshes;
			std::vector<float> m_vertexBufferData;
			std::vector<uint32> m_indexBufferData;
			std::string m_path;

#if DEBUG_TOOLS
			std::string _traceName_vertexPositions;
			std::string _traceName_vertexNormals;
			std::string _traceName_vertexTexCoords;
			std::string _traceName_vBufData;
			std::string _traceName_iBufData;
#endif
		};

		TextureData const& GetTexture(TextureID _texture);
		ModelData const& GetModel(ModelID _model);

		bool LoadModel(std::string _path, ModelID& o_modelID);
	}
}