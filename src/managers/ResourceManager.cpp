#include "ResourceManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <fstream>
#include <absl/container/flat_hash_map.h>
#include <absl/container/inlined_vector.h>
#include <array>

#include <stb_image.h>

#include <sokol_fetch.h>

#include "shaders/main.h"

Core::Resource::ModelID::ValueType nextModelID = 0;

Core::Resource::TextureSampleID defaultTextureID{}; // used for missing textures
Core::Resource::TextureSampleID defaultNormalTextureID{}; // used for missing normal textures
absl::flat_hash_map<Core::Resource::TextureID, Core::Resource::TextureData> textures;
absl::flat_hash_map<Core::Resource::ModelID, Core::Resource::ModelData> models;

Core::Resource::SpriteID::ValueType nextSpriteID = 0;
absl::flat_hash_map<Core::Resource::SpriteID, Core::Resource::SpriteData> sprites;

constexpr usize const g_maxSoundEffects = 128;
constexpr usize const g_maxMusic = 32;

Core::Resource::SoundEffectID::ValueType nextSoundEffectID = 0;
Core::Resource::MusicID::ValueType nextMusicID = 0;
std::array<Core::Resource::SoundEffectData, g_maxSoundEffects> soundEffects;
std::array<Core::Resource::MusicData, g_maxMusic> music;

namespace
{
	constexpr uint8 const g_textureNormalOne = 0xFF;
	constexpr uint8 const g_textureNormalZero = 0xFF / 2;
}

namespace Core
{
	namespace Resource
	{
		//--------------------------------------------------------------------------------
		void Init()
		{
			stbi_set_flip_vertically_on_load(false);
			sfetch_setup(sfetch_desc_t{});
		}

		//--------------------------------------------------------------------------------
		void SetupData()
		{
			{
				sg_image_data emptyTexData{};
				emptyTexData.subimage[0][0] = SG_RANGE(Colour::white);
				sg_image_desc emptyTexDesc{
					.width = 1,
					.height = 1,
					.min_filter = SG_FILTER_LINEAR,
					.mag_filter = SG_FILTER_LINEAR,
					.wrap_u = SG_WRAP_REPEAT,
					.wrap_v = SG_WRAP_REPEAT,
					.data = emptyTexData,
					.label = "default-colour-texture",
				};
				defaultTextureID = sg_make_image(emptyTexDesc);
			}
			{
				uint8 const emptyTex[] = { g_textureNormalZero, g_textureNormalZero, g_textureNormalOne, 0, };
				sg_image_data emptyTexData{};
				emptyTexData.subimage[0][0] = SG_RANGE(emptyTex);
				sg_image_desc emptyTexDesc{
					.width = 1,
					.height = 1,
					.min_filter = SG_FILTER_LINEAR,
					.mag_filter = SG_FILTER_LINEAR,
					.wrap_u = SG_WRAP_REPEAT,
					.wrap_v = SG_WRAP_REPEAT,
					.data = emptyTexData,
					.label = "default-normal-texture",
				};
				defaultNormalTextureID = sg_make_image(emptyTexDesc);
			}
		}

		//--------------------------------------------------------------------------------
		void Cleanup()
		{
			sfetch_shutdown();
		}

		//--------------------------------------------------------------------------------
		ModelID NewModelID() { return nextModelID++; }
		SpriteID NewSpriteID() { return nextSpriteID++; }
		SoundEffectID NewSoundEffectID() { kaAssert(nextSoundEffectID < g_maxSoundEffects, "ran out of sound effects"); return nextSoundEffectID++; }
		MusicID NewMusicID() { kaAssert(nextMusicID < g_maxMusic, "ran out of music"); return nextMusicID++; }

		//--------------------------------------------------------------------------------
		TextureData const& GetTexture(TextureID _texture) { return textures.at(_texture); }
		ModelData const& GetModel(ModelID _model) { return models.at(_model); }
		SpriteData const& GetSprite(SpriteID _sprite) { return sprites.at(_sprite); }
		SoundEffectData& GetSoundEffect(SoundEffectID _soundEffect) { return soundEffects[_soundEffect.GetValue()]; }
		MusicData& GetMusic(MusicID _music) { return music[_music.GetValue()]; }


		//--------------------------------------------------------------------------------
		/// texture
		//--------------------------------------------------------------------------------
		TextureID FindExistingTexture
		(
			std::string const& _texPath
		)
		{
			for (auto const& [textureID, texture] : textures)
			{
				if (texture.m_path == _texPath)
				{
					return textureID;
				}
			}

			return TextureID{};
		}

		bool CheckRGBAForAlpha
		(
			uint8 const* _data,
			usize _dataSize
		)
		{
			for (usize i = 3; i < _dataSize; i += 4)
			{
				if (_data[i] < Colour::componentMax)
				{
					return true;
				}
			}
			return false;
		}

		bool CheckRGBAForSemiTransparency
		(
			uint8 const* _data,
			usize _dataSize
		)
		{
			for (usize i = 3; i < _dataSize; i += 4)
			{
				if (_data[i] > 0 && _data[i] < Colour::componentMax)
				{
					return true;
				}
			}
			return false;
		}

		//--------------------------------------------------------------------------------
		bool LoadCubemapFromFile
		(
			std::string const& _cubemapPath,
			sg_image& o_imageID
		)
		{
			sg_image_desc imageDesc{
				.type = SG_IMAGETYPE_CUBE,
				.pixel_format = SG_PIXELFORMAT_RGBA8,
				.min_filter = SG_FILTER_LINEAR,
				.mag_filter = SG_FILTER_LINEAR,
				.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
				.wrap_v = SG_WRAP_CLAMP_TO_EDGE,
				.wrap_w = SG_WRAP_CLAMP_TO_EDGE,
				.label = _cubemapPath.c_str(),
			};

			std::string const directory = _cubemapPath.substr(0, _cubemapPath.find_last_of('/') + 1);
			std::array<std::string, 6> cubemapFilenames;
			{
				std::ifstream cubemapFile{ _cubemapPath };
				if (!cubemapFile.is_open())
				{
					kaError("failed to open cubemap " + _cubemapPath);
					return false;
				}

				for (usize i = 0; i < 6; ++i)
				{
					if (!std::getline(cubemapFile, cubemapFilenames[i]))
					{
						kaError("not enough lines in cubemap file");
						return false;
					}
					cubemapFilenames[i] = directory + cubemapFilenames[i];
				}
			}

			for (usize i = 0; i < cubemapFilenames.size(); ++i)
			{
				const int dataComponentCount{ 4 };
				int imageComponentCount{ 0 };
				uint8* data = stbi_load(cubemapFilenames[i].c_str(), &imageDesc.width, &imageDesc.height, &imageComponentCount, dataComponentCount);
				if (data != nullptr)
				{
					kaAssert(imageComponentCount > 0);
					usize const dataSize = imageDesc.width * imageDesc.height * dataComponentCount;
					kaAssert(imageComponentCount <= 3 || !CheckRGBAForAlpha(data, dataSize), "cubemap cannot use alpha");

					imageDesc.data.subimage[i][0] = {
						.ptr = data,
						.size = dataSize,
					};
				}
				else
				{
					kaError("Texture failed to load at path: " + cubemapFilenames[i]);
					for (usize j = 0; j < i; ++j)
					{
						stbi_image_free((void*)imageDesc.data.subimage[j][0].ptr);
					}
					stbi_image_free(data);
					return false;
				}
			}

			o_imageID = sg_make_image(imageDesc);
			for (usize i = 0; i < cubemapFilenames.size(); ++i)
			{
				stbi_image_free((void*)imageDesc.data.subimage[i][0].ptr);
			}
			return true;
		}

		//--------------------------------------------------------------------------------
		bool LoadTextureFromFile
		(
			std::string const& _filename,
			sg_image& o_imageID,
			bool& o_semitransparent,
			int& o_width,
			int& o_height
		)
		{
			sg_image_desc imageDesc{
				.generate_mipmaps = true,
				.pixel_format = SG_PIXELFORMAT_RGBA8,
				.min_filter = SG_FILTER_LINEAR_MIPMAP_LINEAR,
				.mag_filter = SG_FILTER_LINEAR,
				.wrap_u = SG_WRAP_REPEAT,
				.wrap_v = SG_WRAP_REPEAT,
				.label = _filename.c_str(),
			};

			const int dataComponentCount{ 4 };
			int imageComponentCount{ 0 };
			uint8* data = stbi_load(_filename.c_str(), &imageDesc.width, &imageDesc.height, &imageComponentCount, dataComponentCount);
			if (data != nullptr)
			{
				kaAssert(imageComponentCount > 0);
				o_width = imageDesc.width;
				o_height = imageDesc.height;

				usize const dataSize = imageDesc.width * imageDesc.height * dataComponentCount;
				o_semitransparent = CheckRGBAForSemiTransparency(data, dataSize);

				imageDesc.data.subimage[0][0] = {
					.ptr = data,
					.size = dataSize,
				};

				o_imageID = sg_make_image(imageDesc);
				stbi_image_free(data);
				return true;
			}

			stbi_image_free(data);
			return false;
		}

		//--------------------------------------------------------------------------------
		bool Load2DTexture
		(
			std::string const& _path,
			TextureID& o_textureID,
			TextureData::Type _type,
			bool* o_semitransparent // = nullptr
		)
		{
			TextureID existingTexture = FindExistingTexture(_path);
			if (existingTexture.IsValid())
			{
				o_textureID = existingTexture;
				return true;
			}

			// if texture hasn't been loaded already, load it
			sg_image imageID;
			bool semitransparent{ false };
			int width{ 0 };
			int height{ 0 };

			bool const loaded = LoadTextureFromFile(_path, imageID, o_semitransparent != nullptr ? *o_semitransparent : semitransparent, width, height);
			if (!loaded)
			{
				kaError("Failed to load 2D texture " + _path);
				return false;
			}

			o_textureID = imageID;
			TextureData& newTextureData = textures[o_textureID];
			newTextureData.m_path = _path;
			newTextureData.m_type = _type;
			newTextureData.m_width = width;
			newTextureData.m_height = height;

			kaLog("New 2D texture " + _path + " loaded!");

			return true;
		}

		//--------------------------------------------------------------------------------
		/// model
		//--------------------------------------------------------------------------------
		ModelID FindExistingModel
		(
			std::string const& _modelPath
		)
		{
			for (auto const& [modelID, model] : models)
			{
				if (model.m_path == _modelPath)
				{
					return modelID;
				}
			}

			return ModelID{};
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

					TextureData::Type textureType = TextureData::Type::General2D;
					switch (_type)
					{
					case aiTextureType_DIFFUSE:
					{
						textureType = TextureData::Type::Diffuse;
						break;
					}
					case aiTextureType_SPECULAR:
					{
						textureType = TextureData::Type::Specular;
						break;
					}
					case aiTextureType_NORMALS:
					{
						textureType = TextureData::Type::Normal;
						break;
					}
					default:
					{
						kaError("tried to load unsupported texture type");
						break;
					}
					}

					TextureID textureID;
					bool semitransparent{ false };
					if (Load2DTexture(filename, textureID, textureType, &semitransparent))
					{
						kaAssert(textureID.IsValid());
						kaAssert(!semitransparent, "semi-transparent textures nyi");
						o_textures.emplace_back(textureID);
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
				if (_mesh->mTextureCoords[0] != nullptr)
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
			o_newMesh.m_bindings.fs_images[SLOT_main_mat_diffuseTex] = defaultTextureID.GetValue();
			o_newMesh.m_bindings.fs_images[SLOT_main_mat_specularTex] = defaultTextureID.GetValue();
			o_newMesh.m_bindings.fs_images[SLOT_main_mat_normalTex] = defaultNormalTextureID.GetValue();
			for (TextureID const& texID : o_newMesh.m_textures)
			{
				TextureData const& tex = GetTexture(texID);
				switch (tex.m_type)
				{
				case TextureData::Type::Diffuse:
				{
					o_newMesh.m_bindings.fs_images[SLOT_main_mat_diffuseTex] = texID.GetValue();
					break;
				}
				case TextureData::Type::Specular:
				{
					o_newMesh.m_bindings.fs_images[SLOT_main_mat_specularTex] = texID.GetValue();
					break;
				}
				case TextureData::Type::Normal:
				{
					o_newMesh.m_bindings.fs_images[SLOT_main_mat_normalTex] = texID.GetValue();
					break;
				}
				case TextureData::Type::General2D:
				case TextureData::Type::Cubemap:
				{
					kaError("shouldn't have gotten this texture type in a material texture load!");
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
				| aiProcess_GenSmoothNormals
				| aiProcess_GenUVCoords
				| aiProcess_FlipWindingOrder
				| aiProcess_JoinIdenticalVertices
				| aiProcess_CalcTangentSpace
			);

			bool const fileLoaded = scene != nullptr && (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == 0 && scene->mRootNode != nullptr;
			if (!fileLoaded)
			{
				kaError(std::string("assimp error: ") + import.GetErrorString());
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

			kaLog("New model " + _path + " loaded!");
			return true;
		}

		bool LoadCubemap
		(
			std::string const& _cubemapPath,
			TextureID& o_cubemapID
		)
		{
			for (auto const& [texID, texData] : textures)
			{
				if (texData.m_type == TextureData::Type::Cubemap && texData.m_path == _cubemapPath)
				{
					o_cubemapID = texID;
					return true;
				}
			}

			sg_image newImageID{};
			if (LoadCubemapFromFile(_cubemapPath, newImageID))
			{
				o_cubemapID = newImageID;
				TextureData& newTexData = textures[o_cubemapID];
				newTexData.m_type = TextureData::Type::Cubemap;
				newTexData.m_path = _cubemapPath;

				kaLog("New cubemap " + _cubemapPath + " loaded!");
				return true;
			}
			return false;
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
			for (auto const& [spriteID, sprite] : sprites)
			{
				if (sprite.m_path == _path)
				{
					o_spriteID = spriteID;
					return true;
				}
			}

			std::ifstream spriteFile{ _path };
			if (!spriteFile.is_open())
			{
				kaError("could not open sprite file: " + _path);
				return false;
			}

			std::string const directory = _path.substr(0, _path.find_last_of('/') + 1);

			o_spriteID = NewSpriteID();
			SpriteData& newSprite = sprites[o_spriteID];
			newSprite.m_path = _path;

			std::string line;

			// line 1: texture file
			int textureWidth{ 0 };
			int textureHeight{ 0 };
			if (std::getline(spriteFile, line))
			{
				std::string const texturePath = directory + line;
				TextureID textureID;
				if (Load2DTexture(texturePath, textureID, TextureData::Type::General2D))
				{
					kaAssert(textureID.IsValid());

					TextureData const& textureData = GetTexture(textureID);

					newSprite.m_texture = textureID;
					textureWidth = textureData.m_width;
					textureHeight = textureData.m_height;
				}
			}
			else
			{
				kaError("sprite file missing texture name");
				return false;
			}

			// line 2: dimensions
			if (std::getline(spriteFile, line))
			{
				usize const mid = line.find_first_of(' ');
				float const width = static_cast<float>(std::atof(line.substr(0, mid).c_str()));
				float const height = static_cast<float>(std::atof(line.substr(mid + 1).c_str()));
				newSprite.m_dimensions = { width, height };
			}
			else
			{
				kaError("sprite file missing dimensions");
				return false;
			}

			// todo: animations?
			// temp: line 3, top-left UV
			if (std::getline(spriteFile, line))
			{
				usize const mid = line.find_first_of(' ');
				float const x = static_cast<float>(std::atof(line.substr(0, mid).c_str()));
				float const y = static_cast<float>(std::atof(line.substr(mid + 1).c_str()));
				newSprite.m_topLeftUV = { x, y };
			}
			else
			{
				kaError("sprite file missing top-left UV");
				return false;
			}

			newSprite.m_dimensionsUV = newSprite.m_dimensions;
			newSprite.m_dimensionsUV.x /= textureWidth;
			newSprite.m_dimensionsUV.y /= textureHeight;
			newSprite.m_topLeftUV.x /= textureWidth;
			newSprite.m_topLeftUV.y /= textureHeight;

			kaLog("New sprite " + _path + " loaded!");
			return true;
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

				kaLog("New sfx " + _path + " loaded!");
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

				kaLog("New bgm " + _path + " loaded!");
				return true;
			}
			return false;
		}

}
}