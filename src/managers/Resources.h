#pragma once

#include "common.h"
#include "ID.h"

#include <vector>
#include <string>
#include <sokol_gfx.h>

struct aiNode;
struct aiScene;
struct aiMesh;

#define USE_INTERLEAVED 1

namespace Core
{
	namespace Resource
	{
		void Init();

		struct TextureIDType {};
		using TextureID = ID<TextureIDType>;

		struct MeshIDType {};
		using MeshID = ID<MeshIDType>;

		struct ModelIDType {};
		using ModelID = ID<ModelIDType>;

		struct Texture
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

		struct Material
		{
			fVec3Data m_diffuseColour;
			fVec3Data m_specularColour;
			fVec3Data m_ambientColour;
			float m_shininess;

			std::vector<TextureID> m_textures;
		};

#if USE_INTERLEAVED
		struct Vertex
		{
			fVec3Data position;
			fVec3Data normal;
			fVec2Data uv{ 0.0f, 0.0f };
		};
#endif

		struct Mesh
		{
#if USE_INTERLEAVED
			std::vector<Vertex> m_vertices;
#else
			// De-interleaved data.
			std::vector<fVec3Data> m_vertexPositions;
			std::vector<fVec3Data> m_vertexNormals;
			std::vector<fVec2Data> m_vertexTexCoords;
#endif

			std::vector<uint16> m_indices;
			Material m_material;

			sg_bindings m_bindings{};
#if DEBUG_TOOLS
			std::string _traceName_vertexPositions;
			std::string _traceName_vertexNormals;
			std::string _traceName_vertexTexCoords;
			std::string _traceName_indices;
#endif
		};
		
		struct Model
		{
			// for now just stores meshes, no transform tree
			std::vector<MeshID> m_meshes;
			std::string m_path;
		};

		Texture const& GetTexture(TextureID _texture);
		Mesh const& GetMesh(MeshID _mesh);
		Model const& GetModel(ModelID _model);

		bool LoadModel(std::string _path, ModelID& o_modelID);
	}
}