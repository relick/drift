#include "ResourceManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <absl/container/flat_hash_map.h>
#include <array>

#include <stb_image.h>

#include "shaders/main.h"

Core::Resource::TextureID::ValueType nextTextureID = 0;
Core::Resource::ModelID::ValueType nextModelID = 0;

sg_image defaultTextureID{}; // used for missing textures
absl::flat_hash_map<Core::Resource::TextureID, Core::Resource::TextureData> textures;
absl::flat_hash_map<Core::Resource::ModelID, Core::Resource::ModelData> models;

constexpr usize const g_maxSoundEffects = 128;
constexpr usize const g_maxMusic = 32;

Core::Resource::SoundEffectID::ValueType nextSoundEffectID = 0;
Core::Resource::MusicID::ValueType nextMusicID = 0;
std::array<Core::Resource::SoundEffectData, g_maxSoundEffects> soundEffects;
std::array<Core::Resource::MusicData, g_maxMusic> music;

namespace Core
{
	namespace Resource
	{
		//--------------------------------------------------------------------------------
		void Init()
		{
			stbi_set_flip_vertically_on_load(true);
		}

		//--------------------------------------------------------------------------------
		void Setup()
		{
			uint8 emptyTex[] = { 255, 255, 255, 255 };
			sg_image_data emptyTexData{};
			emptyTexData.subimage[0][0].ptr = emptyTex;
			emptyTexData.subimage[0][0].size = sizeof(emptyTex);
			sg_image_desc emptyTexDesc{
				.width = 1,
				.height = 1,
				.min_filter = SG_FILTER_LINEAR,
				.mag_filter = SG_FILTER_LINEAR,
				.wrap_u = SG_WRAP_REPEAT,
				.wrap_v = SG_WRAP_REPEAT,
				.data = emptyTexData,
			};
			defaultTextureID = sg_make_image(emptyTexDesc);
		}

		//--------------------------------------------------------------------------------
		void Cleanup()
		{
		}

		//--------------------------------------------------------------------------------
		TextureID NewTextureID() { return nextTextureID++; }
		ModelID NewModelID() { return nextModelID++; }
		SoundEffectID NewSoundEffectID() { ASSERT(nextSoundEffectID < g_maxSoundEffects, "ran out of sound effects"); return nextSoundEffectID++; }
		MusicID NewMusicID() { ASSERT(nextMusicID < g_maxMusic, "ran out of music"); return nextMusicID++; }

		//--------------------------------------------------------------------------------
		TextureData const& GetTexture(TextureID _texture) { return textures.at(_texture); }
		ModelData const& GetModel(ModelID _model) { return models.at(_model); }
		SoundEffectData& GetSoundEffect(SoundEffectID _soundEffect) { return soundEffects[_soundEffect.GetValue()]; }
		MusicData& GetMusic(MusicID _music) { return music[_music.GetValue()]; }

		//--------------------------------------------------------------------------------
		/// sound
		//--------------------------------------------------------------------------------
		bool LoadSoundEffect
		(
			std::string const& _path,
			SoundEffectID& o_soundEffectID
		)
		{
			for (SoundEffectID::ValueType i = 0; i < nextSoundEffectID; ++i)
			{
				if (soundEffects[i].m_path == _path)
				{
					o_soundEffectID = i;
					return true;
				}
			}

			o_soundEffectID = NewSoundEffectID();
			SoundEffectData& newSoundEffect = soundEffects[o_soundEffectID.GetValue()];
			if (newSoundEffect.m_sound.load(_path.c_str()) == SoLoud::SO_NO_ERROR)
			{
				newSoundEffect.m_path = _path;
				return true;
			}
			return false;
		}

		//--------------------------------------------------------------------------------
		bool LoadMusic
		(
			std::string const& _path,
			MusicID& o_musicID
		)
		{
			for (MusicID::ValueType i = 0; i < nextMusicID; ++i)
			{
				if (music[i].m_path == _path)
				{
					o_musicID = i;
					return true;
				}
			}

			o_musicID = NewMusicID();
			MusicData& newMusic = music[o_musicID.GetValue()];
			if (newMusic.m_music.load(_path.c_str()) == SoLoud::SO_NO_ERROR)
			{
				newMusic.m_path = _path;
				return true;
			}
			return false;
		}


		//--------------------------------------------------------------------------------
		/// model
		//--------------------------------------------------------------------------------
		TextureID FindExistingTexture(char const* _texPath)
		{
			for (auto const& texPair : textures)
			{
				if (std::strcmp(texPair.second.m_path.c_str(), _texPath) == 0)
				{
					return texPair.first;
				}
			}

			return TextureID{};
		}

		//--------------------------------------------------------------------------------
		ModelID FindExistingModel(char const* _modelPath)
		{
			for (auto const& modelPair : models)
			{
				if (std::strcmp(modelPair.second.m_path.c_str(), _modelPath) == 0)
				{
					return modelPair.first;
				}
			}

			return ModelID{};
		}

		//--------------------------------------------------------------------------------
		bool LoadTextureFromFile(std::string const& _directory, char const* _path, sg_image& o_imageID, bool _gamma = false)
		{
			std::string const filename = _directory + '/' + _path;

			sg_image_desc imageDesc{};

			int nrComponents;
			uint8* data = stbi_load(filename.c_str(), &imageDesc.width, &imageDesc.height, &nrComponents, 4);
			if (data)
			{
				imageDesc.pixel_format = SG_PIXELFORMAT_RGBA8;
				imageDesc.data.subimage[0][0].ptr = data;
				imageDesc.data.subimage[0][0].size = imageDesc.width * imageDesc.height * nrComponents;

				imageDesc.generate_mipmaps = true;
				imageDesc.min_filter = SG_FILTER_LINEAR_MIPMAP_LINEAR;
				imageDesc.mag_filter = SG_FILTER_LINEAR;
				imageDesc.wrap_u = SG_WRAP_REPEAT;
				imageDesc.wrap_v = SG_WRAP_REPEAT;

				o_imageID = sg_make_image(imageDesc);
				stbi_image_free(data);
				return true;
			}
			else
			{
				std::cout << "Texture failed to load at path: " << filename << std::endl;
				stbi_image_free(data);
				return false;
			}
		}

		//--------------------------------------------------------------------------------
		void LoadMaterialTextures(std::string const& _directory, aiMaterial* _mat, std::vector<TextureID>& o_textures)
		{
			auto fn_loadTextureType = [&_directory, &_mat, &o_textures](aiTextureType _type)
			{
				for (unsigned int i = 0; i < _mat->GetTextureCount(_type); i++)
				{
					aiString str;
					_mat->GetTexture(_type, i, &str);
					TextureID existingTexture = FindExistingTexture(str.C_Str());
					if (existingTexture.IsValid())
					{
						o_textures.push_back(existingTexture);
					}
					else
					{   // if texture hasn't been loaded already, load it
						sg_image imageID;
						bool const loaded = LoadTextureFromFile(_directory, str.C_Str(), imageID);
						if (loaded)
						{
							TextureID const newTextureID = NewTextureID();
							TextureData& newTexture = textures[newTextureID];
							newTexture.m_texID = imageID;
							switch (_type)
							{
							case aiTextureType_DIFFUSE:
							{
								newTexture.m_type = TextureData::Type::Diffuse;
								break;
							}
							case aiTextureType_SPECULAR:
							{
								newTexture.m_type = TextureData::Type::Specular;
								break;
							}
							case aiTextureType_NORMALS:
							{
								newTexture.m_type = TextureData::Type::Normal;
								break;
							}
							default:
							{
								// unsupported texture type!!
								ASSERT(false);
							}
							}
							newTexture.m_path = str.C_Str();
							o_textures.push_back(newTextureID);
						}
					}


					// TODO support more than 1 texture?
					break;
				}
			};

			fn_loadTextureType(aiTextureType_DIFFUSE);
			fn_loadTextureType(aiTextureType_SPECULAR);
			fn_loadTextureType(aiTextureType_NORMALS);
		}

		//--------------------------------------------------------------------------------
		MeshData ProcessMesh(std::string const& _directory, aiMesh* _mesh, aiScene const* _scene)
		{
			MeshData newMesh;
			newMesh.m_loadData = MeshLoadData{};
			// setup allocations
			newMesh.m_loadData->m_vertices.resize(_mesh->mNumVertices);

			static constexpr usize numIndicesPerFace = 3u;
			newMesh.m_loadData->m_indices.resize(_mesh->mNumFaces * numIndicesPerFace);

			// process vertices
			for (uint32 i = 0; i < _mesh->mNumVertices; i++)
			{
				newMesh.m_loadData->m_vertices[i].position = fVec3(_mesh->mVertices[i].x, _mesh->mVertices[i].y, _mesh->mVertices[i].z);
				newMesh.m_loadData->m_vertices[i].normal = fVec3(_mesh->mNormals[i].x, _mesh->mNormals[i].y, _mesh->mNormals[i].z);
				if (_mesh->mTextureCoords[0])
				{
					newMesh.m_loadData->m_vertices[i].uv = fVec2(_mesh->mTextureCoords[0][i].x, _mesh->mTextureCoords[0][i].y);
				}
			}

			for (usize i = 0; i < _mesh->mNumFaces; i++)
			{
				aiFace const& face = _mesh->mFaces[i];
				ASSERT(face.mNumIndices == numIndicesPerFace);
				for (usize j = 0; j < face.mNumIndices; j++)
				{
					newMesh.m_loadData->m_indices[i * numIndicesPerFace + j] = static_cast<uint16>(face.mIndices[j]);
				}
			}

			// process material
			if (_mesh->mMaterialIndex >= 0)
			{
				aiMaterial* material = _scene->mMaterials[_mesh->mMaterialIndex];

				MaterialData& newMaterial = newMesh.m_material;
				LoadMaterialTextures(_directory, material, newMesh.m_textures);

				aiColor3D colour(0.0f, 0.0f, 0.0f);
				float shininess{ 0.0f };

				material->Get(AI_MATKEY_COLOR_DIFFUSE, colour);
				newMaterial.diffuseColour = fVec3(colour.r, colour.g, colour.b);

				material->Get(AI_MATKEY_COLOR_AMBIENT, colour);
				newMaterial.ambientColour = fVec3(colour.r, colour.g, colour.b);

				material->Get(AI_MATKEY_COLOR_SPECULAR, colour);
				newMaterial.specularColour = fVec3(colour.r, colour.g, colour.b);

				material->Get(AI_MATKEY_SHININESS, shininess);
				newMaterial.shininess = shininess;
			}

			// now finalise by making texture bindings
			newMesh.m_bindings.fs_images[SLOT_main_mat_diffuseTex] = defaultTextureID;
			newMesh.m_bindings.fs_images[SLOT_main_mat_specularTex] = defaultTextureID;
			//newMesh.m_bindings.fs_images[SLOT_main_mat_normalTex] = defaultTextureID;
			for (TextureID const& texID : newMesh.m_textures)
			{
				TextureData const& tex = GetTexture(texID);
				switch (tex.m_type)
				{
				case TextureData::Type::Diffuse:
				{
					newMesh.m_bindings.fs_images[SLOT_main_mat_diffuseTex] = tex.m_texID;
					break;
				}
				case TextureData::Type::Specular:
				{
					newMesh.m_bindings.fs_images[SLOT_main_mat_specularTex] = tex.m_texID;
					break;
				}
				case TextureData::Type::Normal:
				{
					// not supported yet
					//newMesh.m_bindings.fs_images[SLOT_main_mat_normalTex] = tex.m_texID;
					break;
				}
				}
			}

			return newMesh;
		}

		//--------------------------------------------------------------------------------
		void ProcessNode(std::string const& _directory, ModelData& io_model, aiNode* _node, aiScene const* _scene)
		{
			// process all the node's meshes (if any)
			for (unsigned int i = 0; i < _node->mNumMeshes; i++)
			{
				aiMesh* mesh = _scene->mMeshes[_node->mMeshes[i]];
				io_model.m_meshes.push_back(ProcessMesh(_directory, mesh, _scene));
			}
			// then do the same for each of its children
			for (unsigned int i = 0; i < _node->mNumChildren; i++)
			{
				ProcessNode(_directory, io_model, _node->mChildren[i], _scene);
			}
		}

		//--------------------------------------------------------------------------------
		bool LoadModel
		(
			std::string const& _path,
			ModelID& o_modelID
		)
		{
			ModelID const existingID = FindExistingModel(_path.c_str());
			if (existingID.IsValid())
			{
				o_modelID = existingID;
				return true;
			}

			Assimp::Importer import;
			aiScene const* scene = import.ReadFile(_path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_GenUVCoords | aiProcess_FlipWindingOrder | aiProcess_JoinIdenticalVertices);

			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			{
				std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
				return false;
			}
			std::string const directory = _path.substr(0, _path.find_last_of('/'));

			o_modelID = NewModelID();
			ModelData& newModel = models[o_modelID];
			newModel.m_loadData = ModelLoadData{};
			newModel.m_path = _path;
			ProcessNode(directory, newModel, scene->mRootNode, scene);

			// Make bindings for model

			// First, fill scratch data.
			usize meshVertexOffset = 0;
			usize meshIndexOffset = 0;
			usize totalVertexCount = 0;
			usize totalIndexCount = 0;
			for (MeshData const& mesh : newModel.m_meshes)
			{
				totalVertexCount += mesh.m_loadData->m_vertices.size();
				totalIndexCount += mesh.m_loadData->m_indices.size();
			}
			newModel.m_loadData->m_vertexBufferData.reserve((sizeof(Resource::VertexData)/sizeof(float)) * totalVertexCount);
			newModel.m_loadData->m_indexBufferData.reserve(totalIndexCount);
			for (MeshData& mesh : newModel.m_meshes)
			{
				for (VertexData const& vertex : mesh.m_loadData->m_vertices)
				{
					newModel.m_loadData->m_vertexBufferData.push_back(vertex.position.x);
					newModel.m_loadData->m_vertexBufferData.push_back(vertex.position.y);
					newModel.m_loadData->m_vertexBufferData.push_back(vertex.position.z);
					newModel.m_loadData->m_vertexBufferData.push_back(vertex.normal.x);
					newModel.m_loadData->m_vertexBufferData.push_back(vertex.normal.y);
					newModel.m_loadData->m_vertexBufferData.push_back(vertex.normal.z);
					newModel.m_loadData->m_vertexBufferData.push_back(vertex.uv.x);
					newModel.m_loadData->m_vertexBufferData.push_back(vertex.uv.y);
				}
				for (auto const& index : mesh.m_loadData->m_indices)
				{
					newModel.m_loadData->m_indexBufferData.push_back(static_cast<uint32>(meshVertexOffset + index));
				}
				mesh.m_bindings.index_buffer_offset = static_cast<int>(meshIndexOffset * sizeof(Resource::IndexType));
				meshVertexOffset += mesh.m_loadData->m_vertices.size();
				meshIndexOffset += mesh.m_loadData->m_indices.size();
			}

			// Now, create buffers and bind to all meshes
			sg_buffer vBuf{};
			sg_buffer iBuf{};
			{
				sg_buffer_desc vBufDesc{};
				vBufDesc.type = SG_BUFFERTYPE_VERTEXBUFFER;
				vBufDesc.data = { &newModel.m_loadData->m_vertexBufferData[0], newModel.m_loadData->m_vertexBufferData.size() * sizeof(float), };
#if DEBUG_TOOLS
				newModel._traceName_vBufData = directory + "/vertices";
				vBufDesc.label = newModel._traceName_vBufData.c_str();
#endif
				vBuf = sg_make_buffer(vBufDesc);
			}

			{
				sg_buffer_desc iBufDesc{};
				iBufDesc.type = SG_BUFFERTYPE_INDEXBUFFER;
				iBufDesc.data = { &newModel.m_loadData->m_indexBufferData[0], newModel.m_loadData->m_indexBufferData.size() * sizeof(Resource::IndexType), };
#if DEBUG_TOOLS
				newModel._traceName_iBufData = directory + "/indices";
				iBufDesc.label = newModel._traceName_iBufData.c_str();
#endif
				iBuf = sg_make_buffer(iBufDesc);
			}
			for (MeshData& mesh : newModel.m_meshes)
			{
				mesh.m_bindings.vertex_buffers[0] = vBuf;
				mesh.m_bindings.index_buffer = iBuf;
			}

			// Clear all loaded data now it's in our gpu
			newModel.CleanData();
			for (MeshData& mesh : newModel.m_meshes)
			{
				mesh.CleanData();
			}

			return true;
		}
	}
}