#pragma once

#include "common.h"

#include <vector>
#include <string>
#include <sokol_gfx.h>

struct aiNode;
struct aiScene;
struct aiMesh;

namespace Core
{
	namespace Resource
	{
		template<typename T_IDType, typename T_Value = uint32>
		class ID
		{
			static constexpr T_Value null_id{ static_cast<T_Value>(-1) };
			T_Value m_id{ null_id };
		public:
			using ValueType = T_Value;
			T_Value GetValue() const { return m_id; }
			bool IsValid() const { return m_id != null_id; }
			bool IsNull() const { return m_id == null_id; }
			template<typename T_OtherIDType>
			bool operator==(ID<T_OtherIDType> const& _b) const { return _b.m_id == m_id; }

			ID() = default;
			ID(T_Value _idvalue) : m_id{ _idvalue } {}
			template<typename T_OtherIDType>
			ID& operator=(ID<T_OtherIDType> const& _b) { m_id = _b.m_id; return *this; }
		};

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

		struct Mesh
		{
			// De-interleaved data.
			std::vector<fVec3Data> m_vertexPositions;
			std::vector<fVec3Data> m_vertexNormals;
			std::vector<fVec2Data> m_vertexTexCoords;

			std::vector<uint16> m_indices;
			Material m_material;

			sg_bindings m_bindings;
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

namespace std
{
	template <typename T_IDType, typename T_Value>
	struct hash<Core::Resource::ID<T_IDType, T_Value>>
	{
		std::size_t operator()(Core::Resource::ID<T_IDType, T_Value> const& _k) const
		{
			return hash<T_Value>()(_k.GetValue());
		}
	};

}