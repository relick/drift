#include "ResourceManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <absl/container/flat_hash_map.h>
#include <absl/container/inlined_vector.h>
#include <array>

#include <stb_image.h>

#include "shaders/main.h"

Core::Resource::TextureID::ValueType nextTextureID = 0;
Core::Resource::ModelID::ValueType nextModelID = 0;

sg_image defaultTextureID{}; // used for missing textures
sg_image defaultNormalTextureID{}; // used for missing normal textures
absl::flat_hash_map<Core::Resource::TextureID, Core::Resource::TextureData> textures;
absl::flat_hash_map<Core::Resource::ModelID, Core::Resource::ModelData> models;

absl::flat_hash_map<Core::Resource::SpriteID, Core::Resource::SpriteData> sprites;

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
			{
				uint8 emptyTex[] = { 128, 128, 255, 255 };
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
				defaultNormalTextureID = sg_make_image(emptyTexDesc);
			}
		}

		//--------------------------------------------------------------------------------
		void Cleanup()
		{
		}

		//--------------------------------------------------------------------------------
		TextureID NewTextureID() { return nextTextureID++; }
		ModelID NewModelID() { return nextModelID++; }
		SoundEffectID NewSoundEffectID() { kaAssert(nextSoundEffectID < g_maxSoundEffects, "ran out of sound effects"); return nextSoundEffectID++; }
		MusicID NewMusicID() { kaAssert(nextMusicID < g_maxMusic, "ran out of music"); return nextMusicID++; }

		//--------------------------------------------------------------------------------
		TextureData const& GetTexture(TextureID _texture) { return textures.at(_texture); }
		ModelData const& GetModel(ModelID _model) { return models.at(_model); }
		SpriteData const& GetSprite(SpriteID _sprite) { return sprites.at(_sprite); }
		SoundEffectData& GetSoundEffect(SoundEffectID _soundEffect) { return soundEffects[_soundEffect.GetValue()]; }
		MusicData& GetMusic(MusicID _music) { return music[_music.GetValue()]; }


		//--------------------------------------------------------------------------------
		/// model
		//--------------------------------------------------------------------------------
		TextureID FindExistingTexture
		(
			std::string const& _texPath
		)
		{
			for (auto const& texPair : textures)
			{
				if (texPair.second.m_path == _texPath)
				{
					return texPair.first;
				}
			}

			return TextureID{};
		}

		//--------------------------------------------------------------------------------
		ModelID FindExistingModel
		(
			std::string const& _modelPath
		)
		{
			for (auto const& modelPair : models)
			{
				if (modelPair.second.m_path == _modelPath)
				{
					return modelPair.first;
				}
			}

			return ModelID{};
		}
		
		bool CheckRGBAForAlpha
		(
			uint8* _data,
			usize _dataSize
		)
		{
			for (usize i = 3; i < _dataSize; i += 4)
			{
				if (_data[i] < 255)
				{
					return true;
				}
			}
			return false;
		}
		
		//--------------------------------------------------------------------------------
		bool LoadTextureFromFile
		(
			std::string const& _filename,
			sg_image& o_imageID,
			bool& o_hasAlpha,
			bool _gamma = false
		)
		{
			sg_image_desc imageDesc{};

			int nrComponents;
			uint8* data = stbi_load(_filename.c_str(), &imageDesc.width, &imageDesc.height, &nrComponents, 4);
			if (data)
			{
				usize const dataSize = imageDesc.width * imageDesc.height * nrComponents;
				o_hasAlpha = CheckRGBAForAlpha(data, dataSize);

				imageDesc.pixel_format = SG_PIXELFORMAT_RGBA8;
				imageDesc.data.subimage[0][0].ptr = data;
				imageDesc.data.subimage[0][0].size = dataSize;

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
				kaError("Texture failed to load at path: " + _filename);
				stbi_image_free(data);
				return false;
			}
		}

		//--------------------------------------------------------------------------------
		void LoadMaterialTextures
		(
			std::string const& _directory,
			aiMaterial* _mat,
			std::vector<TextureID>& o_textures
		)
		{
			auto fn_loadTextureType = [&_directory, &_mat, &o_textures](aiTextureType _type)
			{
				for (unsigned int i = 0; i < _mat->GetTextureCount(_type); i++)
				{
					aiString str;
					_mat->GetTexture(_type, i, &str);

					std::string const filename = _directory + '/' + str.C_Str();

					TextureID existingTexture = FindExistingTexture(filename);
					if (existingTexture.IsValid())
					{
						o_textures.emplace_back(existingTexture);
					}
					else
					{   // if texture hasn't been loaded already, load it
						sg_image imageID;
						bool hasAlpha{ false };
						bool const loaded = LoadTextureFromFile(filename, imageID, hasAlpha);
						if (loaded)
						{
							kaAssert(!hasAlpha, "non-opaque textures nyi");

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
								kaError("tried to load unsupported texture type");
								break;
							}
							}
							newTexture.m_path = filename;
							o_textures.emplace_back(newTextureID);
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
		struct MeshLoadData
		{
			std::vector<VertexData> m_vertices;
			std::vector<IndexType> m_indices;
		};

		struct ModelLoadData
		{
			absl::InlinedVector<float, 256> m_vertexBufferData;
			absl::InlinedVector<uint32, 256> m_indexBufferData;

			absl::InlinedVector<MeshLoadData, 64> m_meshes;
		};

		//--------------------------------------------------------------------------------
		void ProcessMesh
		(
			std::string const& _directory,
			aiMesh* _mesh,
			aiScene const* _scene,
			MeshData& o_newMesh,
			MeshLoadData& o_loadData
		)
		{
			// setup allocations
			o_loadData.m_vertices.resize(_mesh->mNumVertices);

			static constexpr usize numIndicesPerFace = 3u;
			o_loadData.m_indices.resize(_mesh->mNumFaces * numIndicesPerFace);

			// process vertices
			for (uint32 i = 0; i < _mesh->mNumVertices; i++)
			{
				o_loadData.m_vertices[i].position = fVec3(_mesh->mVertices[i].x, _mesh->mVertices[i].y, _mesh->mVertices[i].z);
				o_loadData.m_vertices[i].normal = fVec3(_mesh->mNormals[i].x, _mesh->mNormals[i].y, _mesh->mNormals[i].z);
				if (_mesh->mTextureCoords[0])
				{
					o_loadData.m_vertices[i].uv = fVec2(_mesh->mTextureCoords[0][i].x, _mesh->mTextureCoords[0][i].y);
				}
				o_loadData.m_vertices[i].tangent = fVec3(_mesh->mTangents[i].x, _mesh->mTangents[i].y, _mesh->mTangents[i].z);
			}

			for (usize i = 0; i < _mesh->mNumFaces; i++)
			{
				aiFace const& face = _mesh->mFaces[i];
				kaAssert(face.mNumIndices == numIndicesPerFace);
				for (usize j = 0; j < face.mNumIndices; j++)
				{
					o_loadData.m_indices[i * numIndicesPerFace + j] = static_cast<uint16>(face.mIndices[j]);
				}
			}

			// process material
			if (_mesh->mMaterialIndex >= 0)
			{
				aiMaterial* material = _scene->mMaterials[_mesh->mMaterialIndex];

				MaterialData& newMaterial = o_newMesh.m_material;
				LoadMaterialTextures(_directory, material, o_newMesh.m_textures);

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
			o_newMesh.m_bindings.fs_images[SLOT_main_mat_diffuseTex] = defaultTextureID;
			o_newMesh.m_bindings.fs_images[SLOT_main_mat_specularTex] = defaultTextureID;
			o_newMesh.m_bindings.fs_images[SLOT_main_mat_normalTex] = defaultNormalTextureID;
			for (TextureID const& texID : o_newMesh.m_textures)
			{
				TextureData const& tex = GetTexture(texID);
				switch (tex.m_type)
				{
				case TextureData::Type::Diffuse:
				{
					o_newMesh.m_bindings.fs_images[SLOT_main_mat_diffuseTex] = tex.m_texID;
					break;
				}
				case TextureData::Type::Specular:
				{
					o_newMesh.m_bindings.fs_images[SLOT_main_mat_specularTex] = tex.m_texID;
					break;
				}
				case TextureData::Type::Normal:
				{
					o_newMesh.m_bindings.fs_images[SLOT_main_mat_normalTex] = tex.m_texID;
					break;
				}
				}
			}
		}

		//--------------------------------------------------------------------------------
		void ProcessNode
		(
			std::string const& _directory,
			ModelData& io_model,
			ModelLoadData& io_loadData,
			aiNode* _node,
			aiScene const* _scene
		)
		{
			// process all the node's meshes (if any)
			for (unsigned int i = 0; i < _node->mNumMeshes; i++)
			{
				aiMesh* mesh = _scene->mMeshes[_node->mMeshes[i]];
				io_model.m_meshes.emplace_back();
				io_loadData.m_meshes.emplace_back();
				ProcessMesh(_directory, mesh, _scene, io_model.m_meshes.back(), io_loadData.m_meshes.back());
			}
			// then do the same for each of its children
			for (unsigned int i = 0; i < _node->mNumChildren; i++)
			{
				ProcessNode(_directory, io_model, io_loadData, _node->mChildren[i], _scene);
			}
		}

		//--------------------------------------------------------------------------------
		bool LoadModel
		(
			std::string const& _path,
			ModelID& o_modelID
		)
		{
			ModelID const existingID = FindExistingModel(_path);
			if (existingID.IsValid())
			{
				o_modelID = existingID;
				return true;
			}

			Assimp::Importer import;
			aiScene const* scene = import.ReadFile(
				_path
				, aiProcess_Triangulate
				| aiProcess_FlipUVs
				| aiProcess_GenSmoothNormals
				| aiProcess_GenUVCoords
				| aiProcess_FlipWindingOrder
				| aiProcess_JoinIdenticalVertices
				| aiProcess_CalcTangentSpace
			);

			bool const fileLoaded = scene && (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == 0 && scene->mRootNode;
			if (!fileLoaded)
			{
				std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
				return false;
			}
			std::string const directory = _path.substr(0, _path.find_last_of('/'));

			o_modelID = NewModelID();
			ModelData& newModel = models[o_modelID];
			ModelLoadData loadData;
			newModel.m_path = _path;
			ProcessNode(directory, newModel, loadData, scene->mRootNode, scene);

			kaAssert(newModel.m_meshes.size() == loadData.m_meshes.size());

			// Make bindings for model

			// First, fill scratch data.
			usize meshVertexOffset = 0;
			usize meshIndexOffset = 0;
			usize totalVertexCount = 0;
			usize totalIndexCount = 0;
			for (MeshLoadData const& meshLoadData : loadData.m_meshes)
			{
				totalVertexCount += meshLoadData.m_vertices.size();
				totalIndexCount += meshLoadData.m_indices.size();
			}
			loadData.m_vertexBufferData.reserve((sizeof(Resource::VertexData)/sizeof(float)) * totalVertexCount);
			loadData.m_indexBufferData.reserve(totalIndexCount);
			for (usize meshI = 0; meshI < newModel.m_meshes.size(); ++meshI)
			{
				for (VertexData const& vertex : loadData.m_meshes[meshI].m_vertices)
				{
					loadData.m_vertexBufferData.emplace_back(vertex.position.x);
					loadData.m_vertexBufferData.emplace_back(vertex.position.y);
					loadData.m_vertexBufferData.emplace_back(vertex.position.z);
					loadData.m_vertexBufferData.emplace_back(vertex.normal.x);
					loadData.m_vertexBufferData.emplace_back(vertex.normal.y);
					loadData.m_vertexBufferData.emplace_back(vertex.normal.z);
					loadData.m_vertexBufferData.emplace_back(vertex.uv.x);
					loadData.m_vertexBufferData.emplace_back(vertex.uv.y);
					loadData.m_vertexBufferData.emplace_back(vertex.tangent.x);
					loadData.m_vertexBufferData.emplace_back(vertex.tangent.y);
					loadData.m_vertexBufferData.emplace_back(vertex.tangent.z);
				}
				for (auto const& index : loadData.m_meshes[meshI].m_indices)
				{
					loadData.m_indexBufferData.push_back(static_cast<uint32>(meshVertexOffset + index));
				}
				newModel.m_meshes[meshI].m_bindings.index_buffer_offset = static_cast<int>(meshIndexOffset * sizeof(Resource::IndexType));
				meshVertexOffset += loadData.m_meshes[meshI].m_vertices.size();
				meshIndexOffset += loadData.m_meshes[meshI].m_indices.size();
			}

			// Now, create buffers and bind to all meshes
			sg_buffer vBuf{};
			sg_buffer iBuf{};
			{
				sg_buffer_desc vBufDesc{};
				vBufDesc.type = SG_BUFFERTYPE_VERTEXBUFFER;
				vBufDesc.data = { &loadData.m_vertexBufferData[0], loadData.m_vertexBufferData.size() * sizeof(float), };
#if DEBUG_TOOLS
				newModel._traceName_vBufData = directory + "/vertices";
				vBufDesc.label = newModel._traceName_vBufData.c_str();
#endif
				vBuf = sg_make_buffer(vBufDesc);
			}

			{
				sg_buffer_desc iBufDesc{};
				iBufDesc.type = SG_BUFFERTYPE_INDEXBUFFER;
				iBufDesc.data = { &loadData.m_indexBufferData[0], loadData.m_indexBufferData.size() * sizeof(Resource::IndexType), };
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

			for (usize meshI = 0; meshI < newModel.m_meshes.size(); ++meshI)
			{
				newModel.m_meshes[meshI].SetNumToDraw(static_cast<uint32>(loadData.m_meshes[meshI].m_indices.size()));
			}

			// All loaded data automatically gets cleared now it's in the GPU

			return true;
		}


		//--------------------------------------------------------------------------------
		/// sprite
		//--------------------------------------------------------------------------------
		bool LoadSprite
		(
			std::string const& _path,
			SpriteID& o_spriteID
		)
		{
			return false;
		}


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

	}
}