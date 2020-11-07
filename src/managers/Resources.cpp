#include "Resources.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Core::Resource::TextureID::ValueType nextTextureID = 0;
Core::Resource::MeshID::ValueType nextMeshID = 0;
Core::Resource::ModelID::ValueType nextModelID = 0;

std::unordered_map<Core::Resource::TextureID, Core::Resource::Texture> textures;
std::unordered_map<Core::Resource::MeshID, Core::Resource::Mesh> meshes; 
std::unordered_map<Core::Resource::ModelID, Core::Resource::Model> models;

namespace Core
{
	namespace Resource
	{
		TextureID NewTextureID() { return nextTextureID++; }
		MeshID NewMeshID() { return nextMeshID++; }
		ModelID NewModelID() { return nextModelID++; }

		Texture const& GetTexture(TextureID _texture)
		{
			return textures.at(_texture);
		}
		Mesh const& GetMesh(MeshID _mesh)
		{
			return meshes.at(_mesh);
		}
		Model const& GetModel(ModelID _model)
		{
			return models.at(_model);
		}

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

		bool LoadTextureFromFile(std::string const& _directory, char const* _path, sg_image& o_imageID, bool _gamma = false)
		{
			std::string const filename = _directory + '/' + _path;

			sg_image_desc imageDesc{};

			int nrComponents;
			uint8* data = stbi_load(filename.c_str(), &imageDesc.width, &imageDesc.height, &nrComponents, 4);
			if (data)
			{
				imageDesc.pixel_format = SG_PIXELFORMAT_RGBA8;
				imageDesc.content.subimage[0][0].ptr = data;
				imageDesc.content.subimage[0][0].size = imageDesc.width * imageDesc.height * nrComponents;

				//imageDesc.num_mipmaps = 4;
				//glGenerateMipmap(GL_TEXTURE_2D);
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
							Texture& newTexture = textures[newTextureID];
							newTexture.m_texID = imageID;
							switch (_type)
							{
							case aiTextureType_DIFFUSE:
							{
								newTexture.m_type = Texture::Type::Diffuse;
								break;
							}
							case aiTextureType_SPECULAR:
							{
								newTexture.m_type = Texture::Type::Specular;
								break;
							}
							case aiTextureType_NORMALS:
							{
								newTexture.m_type = Texture::Type::Normal;
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

		Mesh ProcessMesh(std::string const& _directory, aiMesh* _mesh, aiScene const* _scene)
		{
			Mesh newMesh;
			// setup allocations
			newMesh.m_vertexPositions.resize(_mesh->mNumVertices);
			newMesh.m_vertexNormals.resize(_mesh->mNumVertices);
			if (_mesh->mTextureCoords[0])
			{
				newMesh.m_vertexTexCoords.resize(_mesh->mNumVertices);
			}

			static constexpr uint32 numIndicesPerFace = 3;
			newMesh.m_indices.resize(_mesh->mNumFaces * numIndicesPerFace);

			// process vertices
			for (uint32 i = 0; i < _mesh->mNumVertices; i++)
			{
				newMesh.m_vertexPositions[i] = fVec3Data(_mesh->mVertices[i].x, _mesh->mVertices[i].y, _mesh->mVertices[i].z);
				newMesh.m_vertexNormals.emplace_back(_mesh->mNormals[i].x, _mesh->mNormals[i].y, _mesh->mNormals[i].z);
				if (_mesh->mTextureCoords[0])
				{
					newMesh.m_vertexTexCoords.emplace_back(_mesh->mTextureCoords[0][i].x, _mesh->mTextureCoords[0][i].y);
				}
			}

			for (uint32 i = 0; i < _mesh->mNumFaces; i++)
			{
				aiFace const& face = _mesh->mFaces[i];
				ASSERT(face.mNumIndices == numIndicesPerFace);
				for (unsigned int j = 0; j < face.mNumIndices; j++)
				{
					newMesh.m_indices[i * numIndicesPerFace + j] = static_cast<uint16>(face.mIndices[j]);
				}
			}

			// process material
			if (_mesh->mMaterialIndex >= 0)
			{
				aiMaterial* material = _scene->mMaterials[_mesh->mMaterialIndex];

				Material& newMaterial = newMesh.m_material;
				LoadMaterialTextures(_directory, material, newMaterial.m_textures);

				aiColor3D colour(0.0f, 0.0f, 0.0f);
				float shininess{ 0.0f };

				material->Get(AI_MATKEY_COLOR_DIFFUSE, colour);
				newMaterial.m_diffuseColour = fVec3Data(colour.r, colour.b, colour.g);

				material->Get(AI_MATKEY_COLOR_AMBIENT, colour);
				newMaterial.m_ambientColour = fVec3Data(colour.r, colour.b, colour.g);

				material->Get(AI_MATKEY_COLOR_SPECULAR, colour);
				newMaterial.m_specularColour = fVec3Data(colour.r, colour.b, colour.g);

				material->Get(AI_MATKEY_SHININESS, shininess);
				newMaterial.m_shininess = shininess;
			}

			return newMesh;
		}

		void ProcessNode(std::string const& _directory, Model& io_model, aiNode* _node, aiScene const* _scene)
		{
			// process all the node's meshes (if any)
			for (unsigned int i = 0; i < _node->mNumMeshes; i++)
			{
				aiMesh* mesh = _scene->mMeshes[_node->mMeshes[i]];
				MeshID const meshID = NewMeshID();
				meshes.emplace(meshID, ProcessMesh(_directory, mesh, _scene));
				io_model.m_meshes.push_back(meshID);
			}
			// then do the same for each of its children
			for (unsigned int i = 0; i < _node->mNumChildren; i++)
			{
				ProcessNode(_directory, io_model, _node->mChildren[i], _scene);
			}
		}

		bool LoadModel
		(
			std::string _path,
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
			aiScene const* scene = import.ReadFile(_path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_GenUVCoords);

			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			{
				std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
				return false;
			}
			std::string const directory = _path.substr(0, _path.find_last_of('/'));

			ModelID const newModelID = NewModelID();
			Model& newModel = models[newModelID];
			newModel.m_path = _path;
			ProcessNode(directory, newModel, scene->mRootNode, scene);

			return true;
		}
	}
}