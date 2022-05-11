#include "ResourceManager.h"

#include "common/StaticVector.h"

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

static Core::Resource::ModelID::ValueType g_nextModelID = 0;

static Core::Resource::TextureSampleID g_defaultTextureID{}; // used for missing textures
static Core::Resource::TextureSampleID g_defaultNormalTextureID{}; // used for missing normal textures
static absl::flat_hash_map<Core::Resource::TextureID, Core::Resource::TextureData> g_textures;
static absl::flat_hash_map<Core::Resource::ModelID, Core::Resource::ModelData> g_models;

static Core::Resource::SpriteID::ValueType g_nextSpriteID = 0;
static StaticVector<Core::Resource::SpriteID, Core::Resource::SpriteData> g_sprites;

constexpr usize g_maxSoundEffects = 128;
constexpr usize g_maxMusic = 32;

static Core::Resource::SoundEffectID::ValueType g_nextSoundEffectID = 0;
static Core::Resource::MusicID::ValueType g_nextMusicID = 0;
static std::array<Core::Resource::SoundEffectData, g_maxSoundEffects> g_soundEffects;
static std::array<Core::Resource::MusicData, g_maxMusic> g_music;

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
				g_defaultTextureID = sg_make_image(emptyTexDesc);
			}
			{
				auto const emptyTex = std::to_array< uint8 >( { g_textureNormalZero, g_textureNormalZero, g_textureNormalOne, 0, } );
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
				g_defaultNormalTextureID = sg_make_image(emptyTexDesc);
			}
		}

		//--------------------------------------------------------------------------------
		void Cleanup()
		{
			sfetch_shutdown();
		}

		//--------------------------------------------------------------------------------
		static ModelID NewModelID() { return g_nextModelID++; }
		static SoundEffectID NewSoundEffectID() { kaAssert(g_nextSoundEffectID < g_maxSoundEffects, "ran out of sound effects"); return g_nextSoundEffectID++; }
		static MusicID NewMusicID() { kaAssert(g_nextMusicID < g_maxMusic, "ran out of music"); return g_nextMusicID++; }

		//--------------------------------------------------------------------------------
		TextureData const& GetTexture(TextureID _texture) { return g_textures.at(_texture); }
		ModelData const& GetModel(ModelID _model) { return g_models.at(_model); }
		SpriteData const& GetSprite(SpriteID _sprite) { return g_sprites[ _sprite ]; }
		SoundEffectData& GetSoundEffect(SoundEffectID _soundEffect) { return g_soundEffects[_soundEffect.GetValue()]; }
		MusicData& GetMusic(MusicID _music) { return g_music[_music.GetValue()]; }


		//--------------------------------------------------------------------------------
		/// texture
		//--------------------------------------------------------------------------------
		static TextureID FindExistingTexture
		(
			std::string const& _texPath
		)
		{
			for (auto const& [textureID, texture] : g_textures)
			{
				if (texture.m_path == _texPath)
				{
					return textureID;
				}
			}

			return TextureID{};
		}

		static bool CheckRGBAForAlpha
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

		static bool CheckRGBAForSemiTransparency
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
		static bool LoadCubemapFromFile
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
					usize const dataSize = static_cast<usize>(imageDesc.width) * static_cast<usize>(imageDesc.height) * static_cast<usize>(dataComponentCount);
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
						stbi_image_free(const_cast<void*>(imageDesc.data.subimage[j][0].ptr));
					}
					stbi_image_free(data);
					return false;
				}
			}

			o_imageID = sg_make_image(imageDesc);
			for (usize i = 0; i < cubemapFilenames.size(); ++i)
			{
				stbi_image_free(const_cast<void*>(imageDesc.data.subimage[i][0].ptr));
			}
			return true;
		}

		//--------------------------------------------------------------------------------
		static bool LoadTextureFromFile
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

				usize const dataSize = static_cast<usize>(imageDesc.width) * static_cast<usize>(imageDesc.height) * static_cast<usize>(dataComponentCount);
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
		ResourceLoadResult Load2DTexture
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
			TextureData& newTextureData = g_textures[o_textureID];
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
		static ModelID FindExistingModel
		(
			std::string const& _modelPath
		)
		{
			for (auto const& [modelID, model] : g_models)
			{
				if (model.m_path == _modelPath)
				{
					return modelID;
				}
			}

			return ModelID{};
		}
		
		//--------------------------------------------------------------------------------
		static void LoadMaterialTextures
		(
			std::string const& _directory,
			aiMaterial* _mat,
			std::vector<TextureID>& o_textures
		)
		{
			auto fn_loadTextureType = [&_directory, &_mat, &o_textures](aiTextureType _type)
			{
				// TODO support more than 1 texture?
				//for (unsigned int i = 0; i < _mat->GetTextureCount(_type); i++)
				if (unsigned int i = 0; i < _mat->GetTextureCount(_type))
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
			absl::InlinedVector<Vec1, 256> m_vertexBufferData;
			absl::InlinedVector<uint32, 256> m_indexBufferData;

			absl::InlinedVector<MeshLoadData, 64> m_meshes;
		};

		//--------------------------------------------------------------------------------
		static void ProcessMesh
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
				o_loadData.m_vertices[i].position = Vec3(_mesh->mVertices[i].x, _mesh->mVertices[i].y, _mesh->mVertices[i].z);
				o_loadData.m_vertices[i].normal = Vec3(_mesh->mNormals[i].x, _mesh->mNormals[i].y, _mesh->mNormals[i].z);
				if (_mesh->mTextureCoords[0] != nullptr)
				{
					o_loadData.m_vertices[i].uv = Vec2(_mesh->mTextureCoords[0][i].x, _mesh->mTextureCoords[0][i].y);
				}
				o_loadData.m_vertices[i].tangent = Vec3(_mesh->mTangents[i].x, _mesh->mTangents[i].y, _mesh->mTangents[i].z);
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
			{
				aiMaterial* material = _scene->mMaterials[_mesh->mMaterialIndex];

				MaterialData& newMaterial = o_newMesh.m_material;
				LoadMaterialTextures(_directory, material, o_newMesh.m_textures);

				aiColor3D colour(0.0f, 0.0f, 0.0f);
				Vec1 shininess{ 0.0f };

				material->Get(AI_MATKEY_COLOR_DIFFUSE, colour);
				newMaterial.diffuseColour = Vec3(colour.r, colour.g, colour.b);

				material->Get(AI_MATKEY_COLOR_AMBIENT, colour);
				newMaterial.ambientColour = Vec3(colour.r, colour.g, colour.b);

				material->Get(AI_MATKEY_COLOR_SPECULAR, colour);
				newMaterial.specularColour = Vec3(colour.r, colour.g, colour.b);

				material->Get(AI_MATKEY_SHININESS, shininess);
				newMaterial.shininess = shininess;
			}

			// now finalise by making texture bindings
			o_newMesh.m_bindings.fs_images[SLOT_main_mat_diffuseTex] = g_defaultTextureID.GetValue();
			o_newMesh.m_bindings.fs_images[SLOT_main_mat_specularTex] = g_defaultTextureID.GetValue();
			o_newMesh.m_bindings.fs_images[SLOT_main_mat_normalTex] = g_defaultNormalTextureID.GetValue();
			for (TextureID const& texID : o_newMesh.m_textures)
			{
				TextureData const& tex = GetTexture(texID);
				switch (tex.m_type)
				{
					using enum TextureData::Type;

				case Diffuse:
				{
					o_newMesh.m_bindings.fs_images[SLOT_main_mat_diffuseTex] = texID.GetValue();
					break;
				}
				case Specular:
				{
					o_newMesh.m_bindings.fs_images[SLOT_main_mat_specularTex] = texID.GetValue();
					break;
				}
				case Normal:
				{
					o_newMesh.m_bindings.fs_images[SLOT_main_mat_normalTex] = texID.GetValue();
					break;
				}
				case General2D:
				case Cubemap:
				{
					kaError("shouldn't have gotten this texture type in a material texture load!");
					break;
				}
				}
			}
		}

		//--------------------------------------------------------------------------------
		static void ProcessNode
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
		ResourceLoadResult LoadModel
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
			ModelData& newModel = g_models[o_modelID];
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
			loadData.m_vertexBufferData.reserve((sizeof(Resource::VertexData)/sizeof( Vec1 )) * totalVertexCount);
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
				vBufDesc.data = { &loadData.m_vertexBufferData[0], loadData.m_vertexBufferData.size() * sizeof( Vec1 ), };
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
				kaAssert(loadData.m_meshes[meshI].m_indices.size() <= INT_MAX, "Too many vertices want to be rendered in this mesh");
				newModel.m_meshes[meshI].SetNumToDraw(static_cast<int>(loadData.m_meshes[meshI].m_indices.size()));
			}

			// All loaded data automatically gets cleared now it's in the GPU

			kaLog("New model " + _path + " loaded!");
			return true;
		}

		ResourceLoadResult LoadCubemap
		(
			std::string const& _cubemapPath,
			TextureID& o_cubemapID
		)
		{
			for (auto const& [texID, texData] : g_textures)
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
				TextureData& newTexData = g_textures[o_cubemapID];
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
		ResourceLoadResult LoadSprite
		(
			std::string const& _path,
			SpriteID& o_spriteID
		)
		{
			for (auto const& [spriteID, sprite] : g_sprites)
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

			o_spriteID = g_sprites.Emplace();
			SpriteData& newSprite = g_sprites[ o_spriteID ];
			newSprite.m_path = _path;

			std::string line;

			// line 1: texture file
			Vec1 textureWidth{ 1 };
			Vec1 textureHeight{ 1 };
			if (std::getline(spriteFile, line))
			{
				std::string const texturePath = directory + line;
				TextureID textureID;
				if (Load2DTexture(texturePath, textureID, TextureData::Type::General2D))
				{
					kaAssert(textureID.IsValid());

					TextureData const& textureData = GetTexture(textureID);

					newSprite.m_texture = textureID;
					textureWidth = static_cast< Vec1 >(textureData.m_width);
					textureHeight = static_cast< Vec1 >(textureData.m_height);
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
				Vec1 const width = static_cast< Vec1 >(std::atof(line.substr(0, mid).c_str()));
				Vec1 const height = static_cast< Vec1 >(std::atof(line.substr(mid + 1).c_str()));
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
				Vec1 const x = static_cast< Vec1 >(std::atof(line.substr(0, mid).c_str()));
				Vec1 const y = static_cast< Vec1 >(std::atof(line.substr(mid + 1).c_str()));
				newSprite.m_topLeftUV = { x, y };
			}
			else
			{
				kaError("sprite file missing top-left UV");
				return false;
			}
			
			// line 4: alpha
			if ( std::getline( spriteFile, line ) )
			{
				if ( line == "alpha" )
				{
					newSprite.m_useAlpha = true;
				}
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
		ResourceLoadResult LoadSoundEffect
		(
			std::string const& _path,
			SoundEffectID& o_soundEffectID
		)
		{
			for (SoundEffectID::ValueType i = 0; i < g_nextSoundEffectID; ++i)
			{
				if (g_soundEffects[i].m_path == _path)
				{
					o_soundEffectID = i;
					return true;
				}
			}

			o_soundEffectID = NewSoundEffectID();
			SoundEffectData& newSoundEffect = g_soundEffects[o_soundEffectID.GetValue()];
			if (newSoundEffect.m_sound.load(_path.c_str()) == SoLoud::SO_NO_ERROR)
			{
				newSoundEffect.m_path = _path;

				kaLog("New sfx " + _path + " loaded!");
				return true;
			}
			return false;
		}

		//--------------------------------------------------------------------------------
		ResourceLoadResult LoadMusic
		(
			std::string const& _path,
			MusicID& o_musicID
		)
		{
			for (MusicID::ValueType i = 0; i < g_nextMusicID; ++i)
			{
				if (g_music[i].m_path == _path)
				{
					o_musicID = i;
					return true;
				}
			}

			o_musicID = NewMusicID();
			MusicData& newMusic = g_music[o_musicID.GetValue()];
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